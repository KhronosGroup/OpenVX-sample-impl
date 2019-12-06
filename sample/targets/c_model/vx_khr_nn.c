/*
 * Copyright (c) 2016-2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef OPENVX_CONFORMANCE_NEURAL_NETWORKS
#ifdef OPENVX_USE_NN

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <VX/vx_khr_nn.h>

#include "vx_internal.h"
#include <c_model.h>

#include <assert.h>


/****************************************************************************
 *                                                                          *
 *                              Common Utils                                *
 *                                                                          *
 ***************************************************************************/

//TODO: can we use "if(unlikely())"? or an ERR_IFmacro? or must we use
//      if (expr) {} else ? (for compilers like msvc without an unlikely,
//      though it isn't a hot path or anything...)
#define UNLESS(expr_) if (expr_) {} else

#define VX_CALL(expr_)                                  \
    do {                                                \
        vx_status status_ = (expr_);                    \
        UNLESS (status_ == VX_SUCCESS) return status_;  \
    } while(0)

//TODO: move this somewhere, there's like 3 copies of this already
static VX_INLINE int validFormat(vx_enum data_type, vx_uint8 fixed_point_pos)
{
    return
        (data_type == VX_TYPE_INT16 && fixed_point_pos == Q78_FIXED_POINT_POSITION) ||  // Q78
        (data_type == VX_TYPE_UINT8 && !fixed_point_pos) ||                             // U8
        (data_type == VX_TYPE_INT8 && !fixed_point_pos);                                // S8
}

static VX_INLINE enum TensorCFmt getTensorCFmt(vx_tensor tensor)
{
    if (tensor->data_type == VX_TYPE_INT16 && tensor->fixed_point_position == Q78_FIXED_POINT_POSITION)
        return TENSOR_C_FMT_Q78;

    if (tensor->data_type == VX_TYPE_UINT8 && !tensor->fixed_point_position)
        return TENSOR_C_FMT_U8;

    if (tensor->data_type == VX_TYPE_INT8 && !tensor->fixed_point_position)
        return TENSOR_C_FMT_S8;

    assert(0);
    return TENSOR_C_FMT_U8;
}

static VX_INLINE tensor_desc_t getTensorDesc(vx_tensor t)
{
    assert(t);
    tensor_desc_t res = { t->number_of_dimensions, t->dimensions, t->stride };

    return res;
}

static VX_INLINE tensor_desc_t getOptionalTensorDesc(vx_tensor t)
{
    const tensor_desc_t null_tensor_desc = { 0, NULL, NULL };

    return t ? getTensorDesc(t) : null_tensor_desc;
}

// Get a stride satisfying the dimensional relation constraints for
// Convolution/Pooling Nodes.
//
// Note that if there's a range of strides satisfying the requation,
// we choose the minimal one.
// Also note that the dilation definition in OVX may differ from Caffe etc.
static VX_INLINE size_t calcStride(
        bool downscale_roundup,
        size_t input_sz, size_t pad_sz,
        size_t weight_sz, size_t dilation_sz,
        size_t output_sz)
{
    if (input_sz + 2 * pad_sz >= weight_sz + (weight_sz - 1) * dilation_sz && output_sz > 1)
    {
        const size_t a = input_sz + 2 * pad_sz - weight_sz - (weight_sz - 1) * dilation_sz;
        const size_t o = output_sz - 1;

        // We need some stride x that satisfies o = round_div(a, x)
        // since all 3 are positive ints,
        //
        // round_div == ceil    => float_div(a, o) <= x < float_div(a, o-1)
        // round_div == floor   => float_div(a, (o+1)) < x <= float_div(a, o)
        //
        // Since there could be multiple x within this range, we chose the
        // smallest one (@Tomer, this or another similar strategy should be
        // explicitly declared by the spec).
        //
        // And in principle there's no guarantee that any such int x is in
        // the range, but this should be checked @ validation time.

        if (downscale_roundup)
        {
            // Note that since a >= 1, this is guaranteed to be >= 1
            const size_t x = (a + o - 1) / o;
            assert((a + x - 1) / x == o);

            return x;
        }
        else
        {
            const size_t x = a / (o + 1) + 1;
            assert(a / x == o);

            return x;
        }
    }

    //TODO: what should really happen here?
    return 1;
}

static VX_INLINE vx_status scalarCheckGet(
        vx_scalar sc,
        vx_enum type,
        void * ptr)
{
    assert (type > VX_TYPE_INVALID &&
            type <= VX_TYPE_BOOL);

    vx_enum sc_type;
    VX_CALL(vxQueryScalar(sc, VX_SCALAR_TYPE, &sc_type, sizeof(sc_type)));
    UNLESS (sc_type == type) return VX_ERROR_INVALID_PARAMETERS;

    VX_CALL(vxCopyScalar(sc, ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    return VX_SUCCESS;
}


/*==============================================================================
==============================================================================
KERNELS AND INPUT/OUTPUT VALIDATORS
==============================================================================
=============================================================================*/


/****************************************************************************
 *                                                                          *
 *                              vxConvolutionLayer                          *
 *                                                                          *
 ***************************************************************************/

typedef enum _conv_params_e {
    CONV_PARAM_TENSOR_IN,
    CONV_PARAM_WEIGHTS,
    CONV_PARAM_BIASES,
    CONV_PARAM_PAD_X,
    CONV_PARAM_PAD_Y,
    CONV_PARAM_OVERFLOW,
    CONV_PARAM_ROUNDING,
    CONV_PARAM_DOWNSCALE_ROUNDING,
    CONV_PARAM_DILATE_X,
    CONV_PARAM_DILATE_Y,
    CONV_PARAM_TENSOR_OUT,
    CONV_PARAMS_NUMBER
} conv_params_e;

//TODO: verify that the input/output validators support no bia
vx_status VX_CALLBACK nnConvolutionKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)node;

    UNLESS (num == CONV_PARAMS_NUMBER) { return VX_ERROR_INVALID_PARAMETERS; }

    vx_tensor input = (vx_tensor)parameters[CONV_PARAM_TENSOR_IN];
    vx_tensor weights = (vx_tensor)parameters[CONV_PARAM_WEIGHTS];
    vx_tensor biases = (vx_tensor)parameters[CONV_PARAM_BIASES];
    vx_scalar padding_x_sc = (vx_scalar)parameters[CONV_PARAM_PAD_X];
    vx_scalar padding_y_sc = (vx_scalar)parameters[CONV_PARAM_PAD_Y];
    vx_scalar overflow_policy_sc = (vx_scalar)parameters[CONV_PARAM_OVERFLOW];
    vx_scalar rounding_policy_sc = (vx_scalar)parameters[CONV_PARAM_ROUNDING];
    vx_scalar downscale_rounding_sc = (vx_scalar)parameters[CONV_PARAM_DOWNSCALE_ROUNDING];
    vx_scalar dilation_x_sc = (vx_scalar)parameters[CONV_PARAM_DILATE_X];
    vx_scalar dilation_y_sc = (vx_scalar)parameters[CONV_PARAM_DILATE_Y];
    vx_tensor output = (vx_tensor)parameters[CONV_PARAM_TENSOR_OUT];

    vx_enum format;
    vx_size pad_x;
    vx_size pad_y;
    vx_enum overflow;
    vx_enum rounding;
    vx_enum downscale_rounding;
    vx_size dilation_x;
    vx_size dilation_y;
    VX_CALL(vxCopyScalar(padding_x_sc, &pad_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(padding_y_sc, &pad_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(overflow_policy_sc, &overflow, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(rounding_policy_sc, &rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(downscale_rounding_sc, &downscale_rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(dilation_x_sc, &dilation_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(dilation_y_sc, &dilation_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    const bool ceil_round = downscale_rounding == VX_NN_DS_SIZE_ROUNDING_CEILING;
    const size_t stride_x = calcStride(ceil_round, input->dimensions[0], pad_x, weights->dimensions[0], dilation_x, output->dimensions[0]);
    const size_t stride_y = calcStride(ceil_round, input->dimensions[1], pad_y, weights->dimensions[1], dilation_y, output->dimensions[1]);

    VX_CALL(vxQueryTensor(input, VX_TENSOR_DATA_TYPE, &format, sizeof(format)));

    tensor_desc_t input_td = getTensorDesc(input);
    tensor_desc_t weight_td = getTensorDesc(weights);
    tensor_desc_t bias_td = getOptionalTensorDesc(biases);
    tensor_desc_t output_td = getTensorDesc(output);

    enum TensorCFmt fmt = getTensorCFmt(input);
    assert(fmt == getTensorCFmt(weights));
    assert(!biases || fmt == getTensorCFmt(biases));
    assert(fmt == getTensorCFmt(output));

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = calloc(output_td.dims[output_td.dim_num - 1],
            output_td.strides[output_td.dim_num - 1]);
    UNLESS (output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#else
    void * output_ptr = output->addr;
#endif

    ConvolutionKernelImpl(
            fmt,
            input->addr, input_td,
            weights->addr, weight_td,
            (biases ? biases->addr : NULL), bias_td,
            pad_x, pad_y,
            stride_x, stride_y,
            overflow == VX_CONVERT_POLICY_WRAP,
            rounding == VX_ROUND_POLICY_TO_NEAREST_EVEN,
            dilation_x, dilation_y,
            output_ptr, output_td);

    //dumpToFile(outputs3d, vx_true_e);
    //dumpToFile(weights4d, vx_false_e);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    const vx_size view_start[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    status = vxCopyTensorPatch(output, output_td.dim_num, view_start, output_td.dims,
            output_td.strides, output_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    free(output_ptr);

    return status;
#else
    return VX_SUCCESS;
#endif
}


static vx_status VX_CALLBACK nnConvolutionInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_tensor data;
    vx_parameter param;
    vx_size num_of_dims = 0;
    vx_enum data_type = 0;
    vx_enum data_format;
	vx_uint8 fixed_point_pos = 0;
	vx_enum overflow_policy; //WRAP
	vx_enum rounding_policy; //NEAREST_TO_ZERO
    vx_enum down_scale_size_rounding;	// FLOOR
    vx_scalar padding_scalar;
	vx_scalar overflow_policy_sc;
	vx_scalar rounding_policy_sc;
	vx_scalar down_scale_size_rounding_sc;

    switch (index)
    {
    case CONV_PARAM_TENSOR_IN:
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &data, sizeof(data));
        if (data)
        {
            vxQueryTensor(data, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
            vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
            vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_format, sizeof(data_format));
            vxQueryTensor(data, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
            if ((num_of_dims == 3 || num_of_dims == 4) &&
                validFormat(data_format, fixed_point_pos))
            {
                status = VX_SUCCESS;
            }
            vxReleaseTensor(&data);
        }
        vxReleaseParameter(&param);
        break;
    case CONV_PARAM_WEIGHTS:
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &data, sizeof(data));
        if (data)
        {
            vxQueryTensor(data, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
            vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
            vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_format, sizeof(data_format));
            vxQueryTensor(data, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
            if (num_of_dims == 4 && validFormat(data_format, fixed_point_pos))
            {
                status = VX_SUCCESS;
            }
            vxReleaseTensor(&data);
        }
        vxReleaseParameter(&param);
        break;
    case CONV_PARAM_PAD_X:
    case CONV_PARAM_PAD_Y:
        param = vxGetParameterByIndex(node, index);
        status = vxQueryParameter(param, VX_PARAMETER_REF, &padding_scalar, sizeof(padding_scalar));
        // TODO: Add some logic here!
        status |= vxReleaseScalar(&padding_scalar);
        status |= vxReleaseParameter(&param);
        break;
    case CONV_PARAM_OVERFLOW:
    	param  = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &overflow_policy_sc, sizeof(overflow_policy_sc));
        if (overflow_policy_sc)
        {
            vxCopyScalar(overflow_policy_sc, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            if (overflow_policy == VX_CONVERT_POLICY_WRAP ||
                overflow_policy == VX_CONVERT_POLICY_SATURATE)
            {
                status = VX_SUCCESS;
            }
            vxReleaseScalar(&overflow_policy_sc);
        }
        vxReleaseParameter(&param);
    	break;
    case CONV_PARAM_ROUNDING:
      	param = vxGetParameterByIndex(node, index);
        status = vxQueryParameter(param, VX_PARAMETER_REF, &rounding_policy_sc, sizeof(rounding_policy_sc));
        if (rounding_policy_sc)
        {
            status |= vxCopyScalar(rounding_policy_sc, &rounding_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            if (rounding_policy == VX_ROUND_POLICY_TO_ZERO ||
                rounding_policy == VX_ROUND_POLICY_TO_NEAREST_EVEN)
            {
                status |= VX_SUCCESS;
            }
            status |= vxReleaseScalar(&rounding_policy_sc);
        }
        status |= vxReleaseParameter(&param);
    	break;
    case CONV_PARAM_DOWNSCALE_ROUNDING:
      	param = vxGetParameterByIndex(node, index);
        status = vxQueryParameter(param, VX_PARAMETER_REF, &down_scale_size_rounding_sc, sizeof(down_scale_size_rounding_sc));
        if (down_scale_size_rounding_sc)
        {
            status |= vxCopyScalar(down_scale_size_rounding_sc, &down_scale_size_rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            if (down_scale_size_rounding == VX_NN_DS_SIZE_ROUNDING_FLOOR ||
                down_scale_size_rounding == VX_NN_DS_SIZE_ROUNDING_CEILING)
            {
                status |= VX_SUCCESS;
            }
            status |= vxReleaseScalar(&down_scale_size_rounding_sc);
        }
        status |= vxReleaseParameter(&param);
    	break;
  default:
        status = VX_SUCCESS;
        break;
    }

    return status;
}


static vx_status VX_CALLBACK nnConvolutionOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
	vx_parameter param_in, param_weights, param_pad_x, param_pad_y, param_out;
    vx_tensor input, weight,output;
    vx_size num_of_dims_in, num_of_dims_out, num_of_dims_weight, pad_x, pad_y;
	vx_scalar pad_x_s, pad_y_s;
    vx_size dims_in[VX_MAX_TENSOR_DIMENSIONS], dims_weight[VX_MAX_TENSOR_DIMENSIONS], dims_out[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
	vx_enum format_in;
    vx_int8 fixed_point_pos_in;

    if (index == CONV_PARAM_TENSOR_OUT)
    {
        param_in = vxGetParameterByIndex(node, CONV_PARAM_TENSOR_IN);
        param_weights = vxGetParameterByIndex(node, CONV_PARAM_WEIGHTS);
        param_pad_x = vxGetParameterByIndex(node, CONV_PARAM_PAD_X);
		param_pad_y = vxGetParameterByIndex(node, CONV_PARAM_PAD_Y);


		param_out = vxGetParameterByIndex(node, CONV_PARAM_TENSOR_OUT);
		if (param_in && param_weights && param_pad_x&& param_pad_y&& param_out)
        {
            vxQueryParameter(param_in, VX_PARAMETER_REF, &input, sizeof(input));
            vxQueryParameter(param_weights, VX_PARAMETER_REF, &weight, sizeof(weight));
			vxQueryParameter(param_pad_x, VX_PARAMETER_REF, &pad_x_s, sizeof(pad_x_s));
			vxQueryParameter(param_pad_y, VX_PARAMETER_REF, &pad_y_s, sizeof(pad_y_s));
			vxQueryParameter(param_out, VX_PARAMETER_REF, &output, sizeof(output));
			vxCopyScalar(pad_x_s, &pad_x, VX_READ_ONLY,VX_MEMORY_TYPE_HOST);
			vxCopyScalar(pad_y_s, &pad_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
			if (input && weight && output)
            {
                vxQueryTensor(input, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_in, sizeof(num_of_dims_in));
				vxQueryTensor(input, VX_TENSOR_DIMS, dims_in, num_of_dims_in*sizeof(vx_size));
				vxQueryTensor(input, VX_TENSOR_DATA_TYPE, &format_in, sizeof(format_in));
				vxQueryTensor(input, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos_in, sizeof(fixed_point_pos_in));
				vxQueryTensor(output, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_out, sizeof(num_of_dims_out));
				vxQueryTensor(output, VX_TENSOR_DIMS, dims_out, num_of_dims_out*sizeof(vx_size));
				vxQueryTensor(weight, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_weight, sizeof(num_of_dims_weight));
				vxQueryTensor(weight, VX_TENSOR_DIMS, dims_weight, num_of_dims_weight*sizeof(vx_size));

                if (num_of_dims_in>3 && num_of_dims_out>3)
                    dims_out[3] = num_of_dims_in > 3 ? dims_in[3] : 0;
                num_of_dims_out = num_of_dims_in;
                dims_out[2] = dims_weight[3];
                vxSetMetaFormatAttribute(meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_out, sizeof(num_of_dims_out));
                vxSetMetaFormatAttribute(meta, VX_TENSOR_DIMS, dims_out, num_of_dims_out*sizeof(vx_size));
                vxSetMetaFormatAttribute(meta, VX_TENSOR_DATA_TYPE, &format_in, sizeof(format_in));
                vxSetMetaFormatAttribute(meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos_in, sizeof(fixed_point_pos_in));
                status = VX_SUCCESS;

                vxReleaseTensor(&input);
                vxReleaseTensor(&weight);
                vxReleaseTensor(&output);
            }

            vxReleaseParameter(&param_in);
            vxReleaseParameter(&param_weights);
            vxReleaseParameter(&param_pad_x);
            vxReleaseParameter(&param_pad_y);
            vxReleaseParameter(&param_out);

            vxReleaseScalar(&pad_x_s);
            vxReleaseScalar(&pad_y_s);
        }
    	status = VX_SUCCESS;
    }

    return status;
}


static vx_param_description_t nn_convolution_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_OPTIONAL },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t nn_convolution_kernel = {
    VX_KERNEL_CONVOLUTION_LAYER,
    "org.khronos.nn_extension.convolution_layer",
	nnConvolutionKernel,
    nn_convolution_kernel_params, dimof(nn_convolution_kernel_params),
	NULL,
    nnConvolutionInputValidator,
    nnConvolutionOutputValidator,
    NULL,
    NULL,
};


/****************************************************************************
 *                                                                          *
 *                              vxPoolingLayer                              *
 *                                                                          *
 ***************************************************************************/

typedef enum _pool_params_e {
    POOL_PARAM_TENSOR_IN,
    POOL_PARAM_TYPE,
    POOL_PARAM_SIZE_X,
    POOL_PARAM_SIZE_Y,
    POOL_PARAM_PAD_X,
    POOL_PARAM_PAD_Y,
    POOL_PARAM_ROUNDING,
    POOL_PARAM_TENSOR_OUT,
    POOL_PARAMS_NUMBER
} pool_params_e;

static vx_status VX_CALLBACK nnPoolingKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;
    UNLESS (num == POOL_PARAMS_NUMBER) { return VX_ERROR_INVALID_PARAMETERS; }

    vx_tensor inputs = (vx_tensor)parameters[POOL_PARAM_TENSOR_IN];
    vx_scalar pool_type_sc = (vx_scalar)parameters[POOL_PARAM_TYPE];
    vx_scalar size_x_sc = (vx_scalar)parameters[POOL_PARAM_SIZE_X];
    vx_scalar size_y_sc = (vx_scalar)parameters[POOL_PARAM_SIZE_Y];
    vx_scalar pad_x_sc = (vx_scalar)parameters[POOL_PARAM_PAD_X];
    vx_scalar pad_y_sc = (vx_scalar)parameters[POOL_PARAM_PAD_Y];
    vx_scalar rounding_sc = (vx_scalar)parameters[POOL_PARAM_ROUNDING];
    vx_tensor outputs = (vx_tensor)parameters[POOL_PARAM_TENSOR_OUT];

    vx_enum pool_type;
    vx_size size_x, size_y;
    vx_size pad_x, pad_y;
    vx_enum rounding;

    VX_CALL(vxCopyScalar(pool_type_sc, &pool_type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(size_x_sc, &size_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(size_y_sc, &size_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(pad_x_sc, &pad_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(pad_y_sc, &pad_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(rounding_sc, &rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    const bool use_ceil = rounding == VX_NN_DS_SIZE_ROUNDING_CEILING;
    const vx_size stride_x = calcStride(use_ceil, inputs->dimensions[0], pad_x, size_x, 0, outputs->dimensions[0]);
    const vx_size stride_y = calcStride(use_ceil, inputs->dimensions[1], pad_y, size_y, 0, outputs->dimensions[1]);

    vx_enum in_dt, out_dt;
    VX_CALL(vxQueryTensor(inputs, VX_TENSOR_DATA_TYPE, &in_dt, sizeof(in_dt)));
    VX_CALL(vxQueryTensor(outputs, VX_TENSOR_DATA_TYPE, &out_dt, sizeof(out_dt)));

    tensor_desc_t input_td = { inputs->number_of_dimensions, inputs->dimensions, inputs->stride };
    tensor_desc_t output_td = { outputs->number_of_dimensions, outputs->dimensions, outputs->stride };

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = calloc(output_td.dims[output_td.dim_num - 1],
                               output_td.strides[output_td.dim_num - 1]);
    UNLESS (output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#else
    void * output_ptr = outputs->addr;
#endif

    const enum TensorCFmt fmt = getTensorCFmt(inputs);
    assert(fmt == getTensorCFmt(outputs));

    PoolingKernelImpl(
            fmt,
            inputs->addr, input_td,
            pool_type == VX_NN_POOLING_MAX,
            size_x, size_y,
            pad_x, pad_y,
            stride_x, stride_y,
            output_ptr, output_td);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    const vx_size view_start[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_status status = vxCopyTensorPatch(outputs, output_td.dim_num, view_start, output_td.dims,
            output_td.strides, output_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    free(output_ptr);
    return status;
#else
    return VX_SUCCESS;
#endif
}


static vx_status VX_CALLBACK nnPoolingInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_parameter param;
    vx_scalar pool_type_sc;
    vx_enum pool_type;
    vx_size num_of_dims_in = 0;
    vx_tensor input;
    vx_enum data_format;
    vx_uint8 fixed_point_pos = 0;
	vx_enum rounding;
	vx_scalar rounding_sc;

    switch (index)
    {
    case POOL_PARAM_TENSOR_IN:
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        vxQueryTensor(input, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_in, sizeof(num_of_dims_in));
        vxQueryTensor(input, VX_TENSOR_DATA_TYPE, &data_format, sizeof(data_format));
        vxQueryTensor(input, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
        if ((num_of_dims_in == 3 || num_of_dims_in == 4) && validFormat(data_format, fixed_point_pos))
        {
            status = VX_SUCCESS;
        }
        vxReleaseTensor(&input);
        vxReleaseParameter(&param);
        break;
    case POOL_PARAM_TYPE:
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &pool_type_sc, sizeof(pool_type_sc));
        vxCopyScalar(pool_type_sc, &pool_type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        if ((pool_type == VX_NN_POOLING_AVG) ||
            (pool_type == VX_NN_POOLING_MAX))
        {
            status = VX_SUCCESS;
        }
        vxReleaseScalar(&pool_type_sc);
        vxReleaseParameter(&param);
        break;
    case POOL_PARAM_ROUNDING:
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &rounding_sc, sizeof(rounding_sc));
        vxCopyScalar(rounding_sc, &rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        if (rounding == VX_NN_DS_SIZE_ROUNDING_FLOOR || rounding == VX_NN_DS_SIZE_ROUNDING_CEILING)
        {
            status = VX_SUCCESS;
        }
        vxReleaseScalar(&rounding_sc);
        vxReleaseParameter(&param);
        break;
    default:
    	status = VX_SUCCESS;
        break;
    }

    return status;
}


//TODO: there's an issue with releasing stuff here, if there's an error
static vx_status VX_CALLBACK nnPoolingOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (index == POOL_PARAM_TENSOR_OUT)
    {
        vx_parameter param_in = vxGetParameterByIndex(node, POOL_PARAM_TENSOR_IN);
        vx_parameter param_type = vxGetParameterByIndex(node, POOL_PARAM_TYPE);
        vx_parameter param_size_x = vxGetParameterByIndex(node, POOL_PARAM_SIZE_X);
        vx_parameter param_size_y = vxGetParameterByIndex(node, POOL_PARAM_SIZE_Y);
        vx_parameter param_pad_x = vxGetParameterByIndex(node, POOL_PARAM_PAD_X);
        vx_parameter param_pad_y = vxGetParameterByIndex(node, POOL_PARAM_PAD_Y);
        vx_parameter param_rounding = vxGetParameterByIndex(node, POOL_PARAM_ROUNDING);
        vx_parameter param_out = vxGetParameterByIndex(node, POOL_PARAM_TENSOR_OUT);

        if (param_in && param_size_x && param_size_y && param_pad_x && param_pad_y && param_rounding && param_out)
        {
            vx_tensor input;
            vx_scalar type_sc;
            vx_scalar size_x_sc;
            vx_scalar size_y_sc;
            vx_scalar pad_x_sc;
            vx_scalar pad_y_sc;
            vx_scalar rounding_sc;
            vx_tensor output;

            vxQueryParameter(param_in, VX_PARAMETER_REF, &input, sizeof(input));
            vxQueryParameter(param_type, VX_PARAMETER_REF, &type_sc, sizeof(type_sc));
            vxQueryParameter(param_size_x, VX_PARAMETER_REF, &size_x_sc, sizeof(size_x_sc));
            vxQueryParameter(param_size_y, VX_PARAMETER_REF, &size_y_sc, sizeof(size_y_sc));
            vxQueryParameter(param_pad_x, VX_PARAMETER_REF, &pad_x_sc, sizeof(pad_x_sc));
            vxQueryParameter(param_pad_y, VX_PARAMETER_REF, &pad_y_sc, sizeof(pad_y_sc));
            vxQueryParameter(param_rounding, VX_PARAMETER_REF, &rounding_sc, sizeof(rounding_sc));
            vxQueryParameter(param_out, VX_PARAMETER_REF, &output, sizeof(output));

            vx_enum type;
            vx_size size_x;
            vx_size size_y;
            vx_size pad_x;
            vx_size pad_y;
            vx_enum rounding;

            vxCopyScalar(type_sc, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            vxCopyScalar(pad_x_sc, &pad_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            vxCopyScalar(pad_y_sc, &pad_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            vxCopyScalar(size_x_sc, &size_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            vxCopyScalar(size_y_sc, &size_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            vxCopyScalar(rounding_sc, &rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

            if (input)
            {

                vx_size num_of_dims_in;
                vx_size dims_in[VX_MAX_TENSOR_DIMENSIONS];
                vx_enum data_type;
                vx_int8 fixed_pos;
                vx_size num_of_dims_out;
                vx_size dims_out[VX_MAX_TENSOR_DIMENSIONS];

                vxQueryTensor(input, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_in, sizeof(num_of_dims_in));
                vxQueryTensor(input, VX_TENSOR_DIMS, dims_in, num_of_dims_in*sizeof(vx_size));
                vxQueryTensor(input, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
                vxQueryTensor(input, VX_TENSOR_FIXED_POINT_POSITION, &fixed_pos, sizeof(fixed_pos));
                vxQueryTensor(output, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_out, sizeof(num_of_dims_out));
                //                vxCopyScalar(sc_pad_y, &pool_pad_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxQueryTensor(output, VX_TENSOR_DIMS, dims_out, num_of_dims_out*sizeof(vx_size));
                vx_size pool_stride_x = calcStride(rounding, dims_in[0], pad_x, size_x, 0, dims_out[0]);
                vx_size pool_stride_y = calcStride(rounding, dims_in[1], pad_y, size_y, 0, dims_out[1]);
                if (pool_stride_x && pool_stride_y)
                {
                    num_of_dims_out = num_of_dims_in;
                    if (num_of_dims_out>3)
                        dims_out[3] = num_of_dims_in > 3 ? dims_in[3] : 0;
                    dims_out[2] = dims_in[2];
                    //                dims_out[1] = (vx_size)((double)(dims_in[1] + 2*pool_pad_y - pool_size_y) / pool_stride_y + 1);
                    //                dims_out[0] = (vx_size)((double)(dims_in[0] + 2*pool_pad_x - pool_size_x) / pool_stride_x + 1);
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_out, sizeof(num_of_dims_out));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_DIMS, dims_out, num_of_dims_out*sizeof(vx_size));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_pos, sizeof(fixed_pos));
                    status = VX_SUCCESS;
                }

                vxReleaseTensor(&input);
                vxReleaseTensor(&output);
            }

            vxReleaseScalar(&type_sc);
            vxReleaseScalar(&pad_x_sc);
            vxReleaseScalar(&pad_y_sc);
            vxReleaseScalar(&size_x_sc);
            vxReleaseScalar(&size_y_sc);
            vxReleaseScalar(&rounding_sc);
        }

        vxReleaseParameter(&param_in);
        vxReleaseParameter(&param_type);
        vxReleaseParameter(&param_size_x);
        vxReleaseParameter(&param_size_y);
        vxReleaseParameter(&param_pad_x);
        vxReleaseParameter(&param_pad_y);
        vxReleaseParameter(&param_rounding);
        vxReleaseParameter(&param_out);
    }

    return status;
}


static vx_param_description_t nn_pooling_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t nn_pooling_kernel = {
    VX_KERNEL_POOLING_LAYER,
    "org.khronos.nn_extension.pooling_layer",
    nnPoolingKernel,
    nn_pooling_kernel_params, dimof(nn_pooling_kernel_params),
	NULL,
    nnPoolingInputValidator,
    nnPoolingOutputValidator,
    NULL,
    NULL,
};


/*==============================================================================
vxNNFullyConnectedKernel
=============================================================================*/
typedef enum _fc_params_e {
    FC_PARAM_TENSOR_IN = 0,
    FC_PARAM_WEIGHTS,
    FC_PARAM_BIASES,
    FC_PARAM_OVERFLOW,
    FC_PARAM_ROUNDING,
    FC_PARAM_TENSOR_OUT,
    FC_PARAMS_NUMBER
} fc_params_e;

static vx_status VX_CALLBACK nnFullyConnectedKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)node;

    UNLESS (num == FC_PARAMS_NUMBER) { return VX_ERROR_INVALID_PARAMETERS; }

    vx_tensor input = (vx_tensor)parameters[FC_PARAM_TENSOR_IN];
    vx_tensor weights = (vx_tensor)parameters[FC_PARAM_WEIGHTS];
    vx_tensor biases = (vx_tensor)parameters[FC_PARAM_BIASES];
    vx_scalar overflow_policy_sc = (vx_scalar)parameters[FC_PARAM_OVERFLOW];
    vx_scalar rounding_policy_sc = (vx_scalar)parameters[FC_PARAM_ROUNDING];
    vx_tensor output = (vx_tensor)parameters[FC_PARAM_TENSOR_OUT];

    vx_enum overflow;
    vx_enum rounding;
    VX_CALL(vxCopyScalar(overflow_policy_sc, &overflow, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(rounding_policy_sc, &rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));


    tensor_desc_t input_td = getTensorDesc(input);
    tensor_desc_t weight_td = getTensorDesc(weights);
    tensor_desc_t bias_td = getOptionalTensorDesc(biases);
    tensor_desc_t output_td = getTensorDesc(output);

    enum TensorCFmt fmt = getTensorCFmt(input);
    assert(fmt == getTensorCFmt(weights));
    assert(!biases || fmt == getTensorCFmt(biases));
    assert(fmt == getTensorCFmt(output));

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = calloc(output_td.dims[output_td.dim_num - 1],
            output_td.strides[output_td.dim_num - 1]);
    UNLESS (output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#else
    void * output_ptr = output->addr;
#endif

    FullyConnectedKernelImpl(
            fmt,
            input->addr, input_td,
            weights->addr, weight_td,
            (biases ? biases->addr : NULL), bias_td,
            overflow == VX_CONVERT_POLICY_WRAP,
            rounding == VX_ROUND_POLICY_TO_NEAREST_EVEN,
            output_ptr, output_td);

    //dumpToFile(outputs3d, vx_true_e);
    //dumpToFile(weights4d, vx_false_e);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    const vx_size view_start[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    status = vxCopyTensorPatch(output, output_td.dim_num, view_start, output_td.dims,
            output_td.strides, output_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    free(output_ptr);

    return status;
#else
    return VX_SUCCESS;
#endif
}


static vx_status VX_CALLBACK nnFullyConnectedInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_parameter param0, param1, param3;
    vx_enum data_type;
    vx_size in_num_of_dims, num_of_dims = 0;
    vx_size out_dim_num;
    vx_size wt_dims[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_size in_dims[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_size out_dims[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_tensor in, wt, out;
	vx_int8 fixed_point_pos = 0;
	vx_enum overflow_policy; //wrap
	vx_enum rounding_policy; // nearest_to_zero
  	vx_parameter param;
	vx_scalar overflow_policy_sc;
	vx_scalar rounding_policy_sc;

    switch (index)
    {
    case FC_PARAM_TENSOR_IN:
        param0 = vxGetParameterByIndex(node, index);
        vxQueryParameter(param0, VX_PARAMETER_REF, &in, sizeof(in));
        vxQueryTensor(in, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
        vxQueryTensor(in, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
        vxQueryTensor(in, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
        if (num_of_dims >= 1 && validFormat(data_type, fixed_point_pos))
        {
            status = VX_SUCCESS;
        }
        vxReleaseTensor(&in);
        vxReleaseParameter(&param0);
        break;
    case FC_PARAM_WEIGHTS:
        param0 = vxGetParameterByIndex(node, FC_PARAM_TENSOR_IN);
        param1 = vxGetParameterByIndex(node, FC_PARAM_WEIGHTS);
        param3 = vxGetParameterByIndex(node, FC_PARAM_TENSOR_OUT);
        vxQueryParameter(param0, VX_PARAMETER_REF, &in, sizeof(in));
        vxQueryParameter(param1, VX_PARAMETER_REF, &wt, sizeof(wt));
        vxQueryParameter(param3, VX_PARAMETER_REF, &out, sizeof(out));
        vxQueryTensor(wt, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
        vxQueryTensor(in, VX_TENSOR_NUMBER_OF_DIMS, &in_num_of_dims, sizeof(in_num_of_dims));
        vxQueryTensor(out, VX_TENSOR_NUMBER_OF_DIMS, &out_dim_num, sizeof(out_dim_num));
        vxQueryTensor(wt, VX_TENSOR_DIMS, &wt_dims, sizeof(wt_dims));
        vxQueryTensor(in, VX_TENSOR_DIMS, &in_dims, sizeof(in_dims));
        vxQueryTensor(out, VX_TENSOR_DIMS, &out_dims, sizeof(out_dims));
        vxQueryTensor(wt, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
        vxQueryTensor(wt, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
        if (validFormat(data_type, fixed_point_pos))
        {
            status = VX_ERROR_INVALID_PARAMETERS;

            if (out_dim_num == 0) goto weight_check_cleanup;

            const vx_size batch_dim_num = out_dim_num - 1;
            if (batch_dim_num > 3) goto weight_check_cleanup;

            const vx_size core_dim_num = in_num_of_dims - batch_dim_num;

            if (core_dim_num == 1)
            {
                if (num_of_dims == 2 && wt_dims[0] == in_dims[0] && wt_dims[1] == out_dims[0])
                {
                    status = VX_SUCCESS;
                }
            }
            else if (core_dim_num == 3)
            {
                if (num_of_dims == 2)
                {
                    if ((wt_dims[0] == in_dims[0] * in_dims[1] * in_dims[2]) && wt_dims[1] == out_dims[0])
                    {
                        status = VX_SUCCESS;
                    }
                }
                else if (num_of_dims == 4)
                {
                    if ((wt_dims[0] == in_dims[0]) && (wt_dims[1] == in_dims[1]) &&
                        (wt_dims[2] == in_dims[2]) && (wt_dims[3] == out_dims[0]))
                    {
                        status = VX_SUCCESS;
                    }
                }
            }

        }
weight_check_cleanup:
        vxReleaseTensor(&out);
        vxReleaseTensor(&wt);
        vxReleaseTensor(&in);
        vxReleaseParameter(&param3);
        vxReleaseParameter(&param1);
        vxReleaseParameter(&param0);
        break;
    case FC_PARAM_OVERFLOW:
      	param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &overflow_policy_sc, sizeof(overflow_policy_sc));
        if (overflow_policy_sc)
        {
            vxCopyScalar(overflow_policy_sc, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            if (overflow_policy == VX_CONVERT_POLICY_WRAP ||
                overflow_policy == VX_CONVERT_POLICY_SATURATE)
            {
                status = VX_SUCCESS;
            }
        }
        vxReleaseScalar(&overflow_policy_sc);
        vxReleaseParameter(&param);
    	break;
    case FC_PARAM_ROUNDING:
      	param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &rounding_policy_sc, sizeof(rounding_policy_sc));
		vxCopyScalar(rounding_policy_sc, &rounding_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
		if (rounding_policy == VX_ROUND_POLICY_TO_ZERO || rounding_policy == VX_ROUND_POLICY_TO_NEAREST_EVEN)
		{
			status = VX_SUCCESS;
		}
		vxReleaseScalar(&rounding_policy_sc);
		vxReleaseParameter(&param);
    	break;
    default:
        status = VX_SUCCESS;
        break;
    }

    return status;
}


static vx_status VX_CALLBACK nnFullyConnectedOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_parameter param0, param1, param5;
    vx_tensor in, weight, out;
    vx_size num_of_dims_in, num_of_dims_wt, num_of_dims_out = 1;
    vx_size dims_in[VX_MAX_TENSOR_DIMENSIONS] = { 0 }, dims_weight[VX_MAX_TENSOR_DIMENSIONS] = { 0 }, dims_out[VX_MAX_TENSOR_DIMENSIONS] = { 0 };

    if (index == FC_PARAM_TENSOR_OUT)
    {
        param0 = vxGetParameterByIndex(node, FC_PARAM_TENSOR_IN);
        param1 = vxGetParameterByIndex(node, FC_PARAM_WEIGHTS);
        param5 = vxGetParameterByIndex(node, FC_PARAM_TENSOR_OUT);
        vx_enum data_type;
    	vx_uint8 fixed_point_pos = 0;

        if (param0 && param1 && param5)
        {
            vxQueryParameter(param0, VX_PARAMETER_REF, &in, sizeof(in));
            vxQueryParameter(param1, VX_PARAMETER_REF, &weight, sizeof(weight));
            vxQueryParameter(param5, VX_PARAMETER_REF, &out, sizeof(out));
            vxQueryTensor(in, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
            vxQueryTensor(in, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));

            if (in && weight && out)
            {
                vxQueryTensor(in, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_in, sizeof(num_of_dims_in));
				vxQueryTensor(in, VX_TENSOR_DIMS, dims_in, num_of_dims_in*sizeof(vx_size));
                vxQueryTensor(weight, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_wt, sizeof(num_of_dims_wt));
				vxQueryTensor(weight, VX_TENSOR_DIMS, dims_weight, num_of_dims_wt*sizeof(vx_size));
                vxQueryTensor(out, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_out, sizeof(num_of_dims_out));
				vxQueryTensor(out, VX_TENSOR_DIMS, dims_out, num_of_dims_out*sizeof(vx_size));

				vxQueryTensor(in, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
				vxQueryTensor(in, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));

				vxSetMetaFormatAttribute(meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims_out, sizeof(num_of_dims_out));
                vxSetMetaFormatAttribute(meta, VX_TENSOR_DIMS, &dims_out, sizeof(dims_out));
                vxSetMetaFormatAttribute(meta, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
                vxSetMetaFormatAttribute(meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
                if (validFormat(data_type, fixed_point_pos))
                {
                    status = VX_SUCCESS;
                }

            }
            vxReleaseTensor(&out);
            vxReleaseTensor(&weight);
            vxReleaseTensor(&in);
            vxReleaseParameter(&param5);
            vxReleaseParameter(&param1);
            vxReleaseParameter(&param0);
        }
    }

    return status;
}
static vx_param_description_t nn_fully_connected_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_OPTIONAL },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t nn_fully_connected_kernel = {
    VX_KERNEL_FULLY_CONNECTED_LAYER,
    "org.khronos.nn_extension.fully_connected_layer",
    nnFullyConnectedKernel,
    nn_fully_connected_kernel_params, dimof(nn_fully_connected_kernel_params),
	NULL,
    nnFullyConnectedInputValidator,
    nnFullyConnectedOutputValidator,
    NULL,
    NULL,
};


/****************************************************************************
 *                                                                          *
 *                              vxSoftmaxLayer                              *
 *                                                                          *
 ***************************************************************************/

typedef enum _softmax_params_e {
    SOFTMAX_PARAM_TENSOR_IN,
    SOFTMAX_PARAM_TENSOR_OUT,
    SOFTMAX_PARAMS_NUMBER
} softmax_params_e;

static vx_status VX_CALLBACK nnSoftmaxKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;
    UNLESS (num == SOFTMAX_PARAMS_NUMBER) { return VX_ERROR_INVALID_PARAMETERS; }

    vx_tensor input = (vx_tensor)parameters[SOFTMAX_PARAM_TENSOR_IN];
    vx_tensor output = (vx_tensor)parameters[SOFTMAX_PARAM_TENSOR_OUT];

    //TODO: do we need to use the extenral api for the tensors once to validate them, here?

    tensor_desc_t input_td = { input->number_of_dimensions, input->dimensions, input->stride };
    tensor_desc_t output_td = { output->number_of_dimensions, output->dimensions, output->stride };

    enum TensorCFmt fmt = getTensorCFmt(input);
    assert(fmt == getTensorCFmt(output));

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = calloc(output_td.dims[output_td.dim_num - 1],
            output_td.strides[output_td.dim_num - 1]);
    UNLESS (output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#else
    void * output_ptr = outputs->addr;
#endif

    SoftmaxKernelImpl(
            fmt,
            input->addr, input_td,
            output_ptr, output_td);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    const vx_size view_start[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_status status = vxCopyTensorPatch(output, output_td.dim_num, view_start, output_td.dims,
            output_td.strides, output_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    free(output_ptr);
    return status;
#else
    return VX_SUCCESS;
#endif
}

static vx_status VX_CALLBACK nnSoftmaxInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_parameter param;
    vx_enum data_type;
    vx_tensor data;
	vx_uint8 fixed_point_pos = 0;

    if (index == SOFTMAX_PARAM_TENSOR_IN)
    {
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &data, sizeof(data));
        if (data) {
            vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
            vxQueryTensor(data, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
            if (validFormat(data_type, fixed_point_pos))
            {
                status = VX_SUCCESS;
            }
            vxReleaseTensor(&data);
        }
        vxReleaseParameter(&param);
    }

    return status;
}


static vx_status VX_CALLBACK nnSoftmaxOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_parameter param;
    vx_enum data_type;
    vx_size num_of_dims = 0;
    vx_tensor data;
    vx_size dims[VX_MAX_TENSOR_DIMENSIONS];
	vx_uint8 fixed_point_pos = 0;

    if (index == SOFTMAX_PARAM_TENSOR_OUT)
    {
        param = vxGetParameterByIndex(node, SOFTMAX_PARAM_TENSOR_IN);

        if (param)
        {
            vxQueryParameter(param, VX_PARAMETER_REF, &data, sizeof(data));

            if (data)
            {
                vxQueryTensor(data, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
                vxQueryTensor(data, VX_TENSOR_DIMS, &dims, sizeof(dims));
                vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
                vxQueryTensor(data, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
                if (validFormat(data_type, fixed_point_pos))
                {
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_DIMS, &dims, sizeof(dims));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
                    status = VX_SUCCESS;
                }
                vxReleaseTensor(&data);
            }

            vxReleaseParameter(&param);
        }
    }

    return status;
}


static vx_param_description_t nn_softmax_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t nn_softmax_kernel = {
    VX_KERNEL_SOFTMAX_LAYER,
    "org.khronos.nn_extension.softmax_layer",
    nnSoftmaxKernel,
    nn_softmax_kernel_params, dimof(nn_softmax_kernel_params),
	NULL,
	nnSoftmaxInputValidator,
	nnSoftmaxOutputValidator,
    NULL,
    NULL,
};


/*==============================================================================
vxNNNormalizationKernel
=============================================================================*/
typedef enum _norm_params_e {
    NORM_PARAM_TENSOR_IN = 0,
    NORM_PARAM_TYPE,
    NORM_PARAM_SIZE,
    NORM_PARAM_ALPHA,
    NORM_PARAM_BETA,
    NORM_PARAM_TENSOR_OUT,
    NORM_PARAMS_NUMBER
} norm_params_e;

static vx_status VX_CALLBACK nnNormalizationKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)node;

    if (num == NORM_PARAMS_NUMBER)
    {
        vx_tensor inputs = (vx_tensor)parameters[NORM_PARAM_TENSOR_IN];
        vx_scalar type = (vx_scalar)parameters[NORM_PARAM_TYPE];
        vx_scalar normalization_size = (vx_scalar)parameters[NORM_PARAM_SIZE];
        vx_scalar alpha = (vx_scalar)parameters[NORM_PARAM_ALPHA];
        vx_scalar beta = (vx_scalar)parameters[NORM_PARAM_BETA];
        vx_tensor outputs = (vx_tensor)parameters[NORM_PARAM_TENSOR_OUT];
        status = NNNormalizationKernelImpl(inputs, type, normalization_size, alpha, beta, outputs);
    }
    return status;
}


static vx_status VX_CALLBACK nnNormalizationInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_parameter param;
    vx_enum data_type, data_format;
    vx_tensor data;
    vx_scalar norm_type_sc;
    vx_enum norm_type;
	vx_uint8 fixed_point_pos = 0;

    switch (index)
    {
    case NORM_PARAM_TENSOR_IN:
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &data, sizeof(data));
        vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
        vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_format, sizeof(data_format));
        vxQueryTensor(data, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
        if (validFormat(data_type, fixed_point_pos))
        {
            status = VX_SUCCESS;
        }
        vxReleaseTensor(&data);
        vxReleaseParameter(&param);
        break;
    case NORM_PARAM_TYPE:
        param = vxGetParameterByIndex(node, index);
        vxQueryParameter(param, VX_PARAMETER_REF, &norm_type_sc, sizeof(norm_type_sc));
        vxCopyScalar(norm_type_sc, &norm_type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        if ((norm_type == VX_NN_NORMALIZATION_SAME_MAP) ||
            (norm_type == VX_NN_NORMALIZATION_ACROSS_MAPS))
        {
            status = VX_SUCCESS;
        }
        vxReleaseScalar(&norm_type_sc);
        vxReleaseParameter(&param);
        break;
    default:
        status = VX_SUCCESS;
        break;
    }

    return status;
}


static vx_status VX_CALLBACK nnNormalizationOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vx_parameter param;
    vx_enum data_type;
    vx_size num_of_dims = 0;
    vx_tensor data;
    vx_size dims[VX_MAX_TENSOR_DIMENSIONS];
	vx_uint8 fixed_point_pos = 0;

    if (index == NORM_PARAM_TENSOR_OUT)
    {
        param = vxGetParameterByIndex(node, NORM_PARAM_TENSOR_IN);

        if (param)
        {
            vxQueryParameter(param, VX_PARAMETER_REF, &data, sizeof(data));

            if (data)
            {
                vxQueryTensor(data, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
                vxQueryTensor(data, VX_TENSOR_DIMS, &dims, sizeof(dims));
                vxQueryTensor(data, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
                vxQueryTensor(data, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
                if (validFormat(data_type, fixed_point_pos))
                {
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_DIMS, &dims, sizeof(dims));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
                    vxSetMetaFormatAttribute(meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
                    status = VX_SUCCESS;
                }
                vxReleaseTensor(&data);
            }

            vxReleaseParameter(&param);
        }
    }

    return status;
}


static vx_param_description_t nn_norm_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t nn_norm_kernel = {
    VX_KERNEL_NORMALIZATION_LAYER,
    "org.khronos.nn_extension.normalization_layer",
    nnNormalizationKernel,
    nn_norm_kernel_params, dimof(nn_norm_kernel_params),
	NULL,
    nnNormalizationInputValidator,
    nnNormalizationOutputValidator,
    NULL,
    NULL,
};


/****************************************************************************
 *                                                                          *
 *                              vxActivationLayer                           *
 *                                                                          *
 ***************************************************************************/

typedef enum {
    ACTIVATION_PARAM_TENSOR_IN,
    ACTIVATION_PARAM_TYPE,
    ACTIVATION_PARAM_A,
    ACTIVATION_PARAM_B,
    ACTIVATION_PARAM_TENSOR_OUT,
    ACTIVATION_PARAMS_NUMBER
} activation_params_e;

static vx_status VX_CALLBACK nnActivationKernel(
        vx_node node,
        const vx_reference * parameters,
        vx_uint32 num)
{
    (void)node;
    UNLESS (num == ACTIVATION_PARAMS_NUMBER) { return VX_ERROR_INVALID_PARAMETERS; }

    vx_tensor input =   (vx_tensor)parameters[ACTIVATION_PARAM_TENSOR_IN];
    vx_scalar func_sc = (vx_scalar)parameters[ACTIVATION_PARAM_TYPE];
    vx_scalar a_sc =    (vx_scalar)parameters[ACTIVATION_PARAM_A];
    vx_scalar b_sc =    (vx_scalar)parameters[ACTIVATION_PARAM_B];
    vx_tensor output =  (vx_tensor)parameters[ACTIVATION_PARAM_TENSOR_OUT];

    vx_enum func;
    vx_float32 a;
    vx_float32 b;

    VX_CALL(vxCopyScalar(func_sc, &func, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(a_sc, &a, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(b_sc, &b, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    //TODO: do we need to use the extenral api for the tensors once to validate them, here?

    tensor_desc_t in_td = getTensorDesc(input);
    tensor_desc_t out_td = getTensorDesc(output);

    enum TensorCFmt fmt = getTensorCFmt(input);
    assert(fmt == getTensorCFmt(output));

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = calloc(out_td.dims[out_td.dim_num - 1],
            out_td.strides[out_td.dim_num - 1]);
    UNLESS (output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#else
    void * output_ptr = output->addr;
#endif

    ActivationKernelImpl(
            fmt,
            input->addr, in_td,
            func, a, b,
            output_ptr, out_td);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    const vx_size view_start[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_status status = vxCopyTensorPatch(output, out_td.dim_num, view_start, out_td.dims,
            out_td.strides, output_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    free(output_ptr);
    return status;
#else
    return VX_SUCCESS;
#endif
}

static vx_status VX_CALLBACK nnActivationValidator(
        vx_node node,
        const vx_reference parameters[],
        vx_uint32 num,
        vx_meta_format metas[])
{
    (void)node;
    UNLESS (num == ACTIVATION_PARAMS_NUMBER) return VX_ERROR_INVALID_PARAMETERS;

    vx_tensor input =   (vx_tensor)parameters[ACTIVATION_PARAM_TENSOR_IN];
    vx_scalar func_sc = (vx_scalar)parameters[ACTIVATION_PARAM_TYPE];
    vx_scalar a_sc =    (vx_scalar)parameters[ACTIVATION_PARAM_A];
    vx_scalar b_sc =    (vx_scalar)parameters[ACTIVATION_PARAM_B];
    vx_tensor output =  (vx_tensor)parameters[ACTIVATION_PARAM_TENSOR_OUT];

    vx_enum func;
    vx_float32 a;
    vx_float32 b;

    VX_CALL(scalarCheckGet(func_sc, VX_TYPE_ENUM, &func));
    VX_CALL(scalarCheckGet(a_sc, VX_TYPE_FLOAT32, &a));
    VX_CALL(scalarCheckGet(b_sc, VX_TYPE_FLOAT32, &b));

    UNLESS (func == VX_NN_ACTIVATION_LOGISTIC ||
            func == VX_NN_ACTIVATION_HYPERBOLIC_TAN ||
            func == VX_NN_ACTIVATION_RELU ||
            func == VX_NN_ACTIVATION_BRELU ||
            func == VX_NN_ACTIVATION_SOFTRELU ||
            func == VX_NN_ACTIVATION_ABS ||
            func == VX_NN_ACTIVATION_SQUARE ||
            func == VX_NN_ACTIVATION_SQRT ||
            func == VX_NN_ACTIVATION_LINEAR)
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layer func param must be LOGISTIC, HYPERBOLIC_TAN, RELU, BRELU, SOFTRELU, ABS, SQUARE, SQRT or, LINEAR");
        return VX_ERROR_INVALID_PARAMETERS;
    }

    UNLESS (func == VX_NN_ACTIVATION_LOGISTIC ||
            func == VX_NN_ACTIVATION_HYPERBOLIC_TAN ||
            func == VX_NN_ACTIVATION_RELU)
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layer only supports LOGISTIC, HYPERBOLIC_TAN and RELU, atm");
        return VX_ERROR_NOT_SUPPORTED;
    }

    //TODO: this is really awkward, the description vx_khr_nn.h says both must be >=0, but why??
    UNLESS (a >= 0 && b >= 0)
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layer expects both a and b to be >= 0 (Even when not used, atm)");
        return VX_ERROR_INVALID_PARAMETERS;
    }

    vx_enum i_dt, o_dt;
    VX_CALL(vxQueryTensor(input, VX_TENSOR_DATA_TYPE, &i_dt, sizeof(i_dt)));
    VX_CALL(vxQueryTensor(output, VX_TENSOR_DATA_TYPE, &o_dt, sizeof(o_dt)));

    vx_uint8 i_fpp, o_fpp;
    VX_CALL(vxQueryTensor(input, VX_TENSOR_FIXED_POINT_POSITION, &i_fpp, sizeof(i_fpp)));
    VX_CALL(vxQueryTensor(output, VX_TENSOR_FIXED_POINT_POSITION, &o_fpp, sizeof(o_fpp)));

    UNLESS ((o_dt == VX_TYPE_INT16 && o_fpp == Q78_FIXED_POINT_POSITION) ||
            (o_dt == VX_TYPE_INT8 && !o_fpp) ||
            (o_dt == VX_TYPE_UINT8 && !o_fpp))
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layer only supports Q78, U8 and S8 formats");
        return VX_ERROR_INVALID_FORMAT;
    }

    UNLESS (i_dt == o_dt)
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layer requires matching tensor data_types");
        return VX_ERROR_INVALID_FORMAT;
    }

    UNLESS (i_fpp == o_fpp)
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layer requires matching tensor fixed_point_position");
        return VX_ERROR_INVALID_FORMAT;
    }

    tensor_desc_t in_td = getTensorDesc(input);
    tensor_desc_t out_td = getTensorDesc(output);

    UNLESS (in_td.dim_num == out_td.dim_num && in_td.dim_num <= 4)
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layer requires (1 <= input.dim_num = output.dim_num <= 4)");
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (in_td.dims[0] == out_td.dims[0] &&
            ((in_td.dim_num < 2 || in_td.dims[1] == out_td.dims[1]) ||
            (in_td.dim_num < 3 || in_td.dims[2] == out_td.dims[2]) ||
            (in_td.dim_num < 4 || in_td.dims[3] == out_td.dims[3])))
    {
        VX_PRINT(VX_ZONE_ERROR, "Activation layers input and output tensor shape must be identical");
        return VX_ERROR_INVALID_DIMENSION;
    }

    vx_meta_format * meta = &metas[ACTIVATION_PARAM_TENSOR_OUT];
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &out_td.dim_num, sizeof(out_td.dim_num)));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_td.dims, sizeof(*out_td.dims) * out_td.dim_num));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &o_dt, sizeof(o_dt)));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &o_fpp, sizeof(o_fpp)));
    return VX_SUCCESS;
}

static vx_param_description_t nn_activation_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

vx_kernel_description_t nn_activation_kernel = {
    VX_KERNEL_ACTIVATION_LAYER,
    "org.khronos.nn_extension.activation_layer",
    nnActivationKernel,
    nn_activation_kernel_params, dimof(nn_activation_kernel_params),
    nnActivationValidator,
    NULL, NULL, NULL, NULL,
};


/****************************************************************************
 *                                                                          *
 *                              vxROIPoolingLayer                           *
 *                                                                          *
 ***************************************************************************/

typedef enum _roipooling_params_e {
    ROIPOOLING_PARAM_TENSOR_IN = 0,
    ROIPOOLING_PARAM_ROIS,
    ROIPOOLING_PARAM_TYPE,
    ROIPOOLING_PARAM_TENSOR_OUT,
    ROIPOOLING_PARAMS_NUMBER
} roipooling_params_e;

static vx_status VX_CALLBACK nnROIPoolingKernel(
        vx_node node,
        const vx_reference * parameters,
        vx_uint32 num)
{
    (void)node;
    UNLESS (num == ROIPOOLING_PARAMS_NUMBER) return VX_ERROR_INVALID_PARAMETERS;

    vx_tensor in0 = (vx_tensor)parameters[ROIPOOLING_PARAM_TENSOR_IN];
    vx_tensor in1 = (vx_tensor)parameters[ROIPOOLING_PARAM_ROIS];
    // note that param 2 isn't used since only MAX pool is supported
    vx_tensor out = (vx_tensor)parameters[ROIPOOLING_PARAM_TENSOR_OUT];

    vx_enum in0_dt, in1_dt, out_dt;
    VX_CALL(vxQueryTensor(in0, VX_TENSOR_DATA_TYPE, &in0_dt, sizeof(in0_dt)));
    VX_CALL(vxQueryTensor(in1, VX_TENSOR_DATA_TYPE, &in1_dt, sizeof(in1_dt)));
    VX_CALL(vxQueryTensor(out, VX_TENSOR_DATA_TYPE, &out_dt, sizeof(out_dt)));

    vx_uint8 in0_fpp;
    vx_uint8 in1_fpp;
    vx_uint8 out_fpp;
    VX_CALL(vxQueryTensor(in0, VX_TENSOR_FIXED_POINT_POSITION, &in0_fpp, sizeof(in0_fpp)));
    VX_CALL(vxQueryTensor(in1, VX_TENSOR_FIXED_POINT_POSITION, &in1_fpp, sizeof(in1_fpp)));
    VX_CALL(vxQueryTensor(out, VX_TENSOR_FIXED_POINT_POSITION, &out_fpp, sizeof(out_fpp)));

    tensor_desc_t in0_td = getTensorDesc(in0);
    tensor_desc_t in1_td = getTensorDesc(in1);
    tensor_desc_t out_td = getTensorDesc(out);

    const enum TensorCFmt fmt = getTensorCFmt(in0);
    assert (getTensorCFmt(in1) == fmt);
    assert (getTensorCFmt(out) == fmt);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = calloc(out_td.dims[out_td.dim_num - 1],
            out_td.strides[out_td.dim_num - 1]);
    UNLESS (output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#else
    void * output_ptr = out->addr;
#endif

    ROIPoolingKernelImpl(
            fmt,
            in0->addr, in0_td,
            in1->addr, in1_td,
            output_ptr, out_td);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    const vx_size view_start[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_status status = vxCopyTensorPatch(out, out_td.dim_num, view_start, out_td.dims,
            out_td.strides, output_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    free(output_ptr);

    return status;
#else
    return VX_SUCCESS;
#endif
}

static vx_status VX_CALLBACK nnROIPoolingValidator(
        vx_node node,
        const vx_reference parameters[],
        vx_uint32 num,
        vx_meta_format metas[])
{
    (void)node;
    UNLESS (num == ROIPOOLING_PARAMS_NUMBER) return VX_ERROR_INVALID_PARAMETERS;

    vx_tensor input_data = (vx_tensor)parameters[ROIPOOLING_PARAM_TENSOR_IN];
    vx_tensor input_rois = (vx_tensor)parameters[ROIPOOLING_PARAM_ROIS];
    vx_scalar pool_type_sc = (vx_scalar)parameters[ROIPOOLING_PARAM_TYPE];
    vx_tensor output_arr = (vx_tensor)parameters[ROIPOOLING_PARAM_TENSOR_OUT];

    {
        vx_enum sc_type;
        VX_CALL(vxQueryScalar(pool_type_sc, VX_SCALAR_TYPE, &sc_type, sizeof(sc_type)));
        UNLESS (sc_type == VX_TYPE_ENUM) return VX_ERROR_INVALID_PARAMETERS;

        vx_enum pool_type;
        VX_CALL(vxCopyScalar(pool_type_sc, &pool_type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
        UNLESS (pool_type == VX_NN_POOLING_MAX)
        {
            VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer only supports VX_NN_POOLING_MAX (%d) atm (got %d)",
                     VX_NN_POOLING_MAX, pool_type);
            return VX_ERROR_NOT_SUPPORTED;
        }
    }

    //TODO: what's the proper way to use an internal API here?

    vx_enum in0_dt, in1_dt, out_dt;
    VX_CALL(vxQueryTensor(input_data, VX_TENSOR_DATA_TYPE, &in0_dt, sizeof(in0_dt)));
    VX_CALL(vxQueryTensor(input_rois, VX_TENSOR_DATA_TYPE, &in1_dt, sizeof(in1_dt)));
    VX_CALL(vxQueryTensor(output_arr, VX_TENSOR_DATA_TYPE, &out_dt, sizeof(out_dt)));

    vx_uint8 in0_fpp;
    vx_uint8 in1_fpp;
    vx_uint8 out_fpp;
    VX_CALL(vxQueryTensor(input_data, VX_TENSOR_FIXED_POINT_POSITION, &in0_fpp, sizeof(in0_fpp)));
    VX_CALL(vxQueryTensor(input_rois, VX_TENSOR_FIXED_POINT_POSITION, &in1_fpp, sizeof(in1_fpp)));
    VX_CALL(vxQueryTensor(output_arr, VX_TENSOR_FIXED_POINT_POSITION, &out_fpp, sizeof(out_fpp)));

    UNLESS ((out_dt == VX_TYPE_INT16 && out_fpp == Q78_FIXED_POINT_POSITION) ||
            (out_dt == VX_TYPE_INT8 && !out_fpp) ||
            (out_dt == VX_TYPE_UINT8 && !out_fpp))
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer only supports Q78, U8 and S8 formats"
               /*" (output_arr had data_type: %d, and fix_point_pos: %d)", out_dt, out_fpp*/);
        return VX_ERROR_INVALID_FORMAT;
    }

    // Note: In principle the node can enforce format matching beween -
    //       input_data an ouput_arr only - Since input_rois values are only
    //       used as coordinates into input_data, there's no requirement for
    //       for the extra restriction.

    UNLESS (in0_dt == in1_dt && in0_dt == out_dt)
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer requires matching tensor data_types"
                /*" (got { %d, %d, %d })", in0_dt, in1_dt, out_dt*/);
        return VX_ERROR_INVALID_FORMAT;
    }
    UNLESS (in0_fpp == in1_fpp && in0_fpp == out_fpp)
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer requires matching tensor fixed_point_position"
               /*" (got { %d, %d, %d })", in0_fpp, in1_fpp, out_fpp*/);
        return VX_ERROR_NOT_SUPPORTED;
    }

    // Note: It's trivial to extend the node to support D-dim ROIs as well as
    //       arbitrary dims for the Chans and Batching.

    //TODO: should we get these values through the external API for some reason here?
    tensor_desc_t in0_td = getTensorDesc(input_data);
    tensor_desc_t in1_td = getTensorDesc(input_rois);
    tensor_desc_t out_td = getTensorDesc(output_arr);

    UNLESS ((in0_td.dim_num == 3 && in1_td.dim_num == 2 && out_td.dim_num == 4) ||
            (in0_td.dim_num == 4 && in1_td.dim_num == 3 && out_td.dim_num == 5))
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer requires the dim num for { input_data, input_rois and output_rois } to be either { 3, 2, 4 } or { 4, 3, 5 }"
                /*" (got { %zu, %zu, %zu })", in0_td.dim_num, in1_td.dim_num, out_td.dim_num*/);
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (in0_td.dim_num == 3 ||
            (in0_td.dims[3] == in1_td.dims[2] && in0_td.dims[3] == out_td.dims[4]))
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer requires matching batching dims when present"
               /*" (got { %zu, %zu, %zu }", in0_td.dims[3], in1_td.dims[2], out_td.dims[4])*/);
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (in1_td.dims[0] == 4)
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer requires input_rois.dims[0] == 4 for 2D rois"
                /*" (got { %zu } )", in0_td.dims[0]*/);
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (in1_td.dims[1] == out_td.dims[3])
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer roi # mismatch between input_rois.dims[1] and output_arr.dims[3]"
                /*" (got { %zu, %zu })", in1_td.dims[1], out_td.dims[3]*/);
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (in0_td.dims[2] == out_td.dims[2])
    {
        VX_PRINT(VX_ZONE_ERROR, "ROIPooling layer channel # mismatch between input_data.dims[2] and output_arr.dims[2]"
                /*" (got { %zu, %zu })", in0_td.dim[2], out_td.dims[2]*/);
        return VX_ERROR_INVALID_DIMENSION;
    }

    VX_CALL(vxSetMetaFormatAttribute(metas[3], VX_TENSOR_NUMBER_OF_DIMS, &out_td.dim_num, sizeof(out_td.dim_num)));
    VX_CALL(vxSetMetaFormatAttribute(metas[3], VX_TENSOR_DIMS, out_td.dims, sizeof(*out_td.dims) * out_td.dim_num));
    VX_CALL(vxSetMetaFormatAttribute(metas[3], VX_TENSOR_DATA_TYPE, &out_dt, sizeof(out_dt)));
    VX_CALL(vxSetMetaFormatAttribute(metas[3], VX_TENSOR_FIXED_POINT_POSITION, &out_fpp, sizeof(out_fpp)));

    return VX_SUCCESS;
}

static vx_param_description_t nn_roipooling_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },  // input_data
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },  // input_rois
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },  // pool_type
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED }, // output_arr
};

vx_kernel_description_t nn_roipooling_kernel = {
    VX_KERNEL_ROI_POOLING_LAYER,
    "org.khronos.nn_extension.roi_pooling_layer",
    nnROIPoolingKernel,
    nn_roipooling_kernel_params, dimof(nn_roipooling_kernel_params),
    nnROIPoolingValidator,
    NULL, NULL, NULL, NULL,
};


/****************************************************************************
 *                                                                          *
 *                              vxDeconvolutionLayer                        *
 *                                                                          *
 ***************************************************************************/

typedef enum {
    DECONV_PARAM_TENSOR_IN,
    DECONV_PARAM_WEIGHTS,
    DECONV_PARAM_BIASES,
    DECONV_PARAM_PAD_X,
    DECONV_PARAM_PAD_Y,
    DECONV_PARAM_OVERFLOW,
    DECONV_PARAM_ROUNDING,
    DECONV_PARAM_A_X,
    DECONV_PARAM_A_Y,
    DECONV_PARAM_TENSOR_OUT,
    DECONV_PARAMS_NUMBER
} deconv_params_e;

static vx_status VX_CALLBACK nnDeconvolutionKernel(
        vx_node node,
        const vx_reference * parameters,
        vx_uint32 num)
{
    (void)node;
    UNLESS (num == DECONV_PARAMS_NUMBER) return VX_ERROR_INVALID_PARAMETERS;

    vx_tensor input =       (vx_tensor)parameters[DECONV_PARAM_TENSOR_IN];
    vx_tensor weights =     (vx_tensor)parameters[DECONV_PARAM_WEIGHTS];
    vx_tensor biases =      (vx_tensor)parameters[DECONV_PARAM_BIASES];
    vx_scalar pad_x_sc =    (vx_scalar)parameters[DECONV_PARAM_PAD_X];
    vx_scalar pad_y_sc =    (vx_scalar)parameters[DECONV_PARAM_PAD_Y];
    vx_scalar overflow_sc = (vx_scalar)parameters[DECONV_PARAM_OVERFLOW];
    vx_scalar rounding_sc = (vx_scalar)parameters[DECONV_PARAM_ROUNDING];
    vx_scalar a_x_sc =      (vx_scalar)parameters[DECONV_PARAM_A_X];
    vx_scalar a_y_sc =      (vx_scalar)parameters[DECONV_PARAM_A_Y];
    vx_tensor output =      (vx_tensor)parameters[DECONV_PARAM_TENSOR_OUT];

    vx_size pad_x;
    vx_size pad_y;
    vx_enum overflow;
    vx_enum rounding;
    vx_size a_x;
    vx_size a_y;

    VX_CALL(vxCopyScalar(pad_x_sc, &pad_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(pad_y_sc, &pad_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(overflow_sc, &overflow, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(rounding_sc, &rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(a_x_sc, &a_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(a_y_sc, &a_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    tensor_desc_t in_td = getTensorDesc(input);
    tensor_desc_t weight_td = getTensorDesc(weights);
    tensor_desc_t bias_td = getOptionalTensorDesc(biases);
    tensor_desc_t out_td = getTensorDesc(output);

    vx_size upscale_x, upscale_y;
    {
        assert (output->dimensions[0] + 2 * pad_x > weights->dimensions[0] + a_x);
        assert (output->dimensions[1] + 2 * pad_y + weights->dimensions[1] + a_y);
        assert (input->dimensions[0] > 1);
        assert (input->dimensions[1] > 1);

        upscale_x = (output->dimensions[0] + 2 * pad_x - weights->dimensions[0] - a_x)/(input->dimensions[0] - 1);
        upscale_y = (output->dimensions[1] + 2 * pad_y - weights->dimensions[1] - a_y)/(input->dimensions[1] - 1);

        assert (upscale_x > 0);
        assert (upscale_y > 0);
        assert (output->dimensions[0] == (input->dimensions[0] - 1) * upscale_x + a_x + weights->dimensions[0] - 2 * pad_x);
        assert (output->dimensions[1] == (input->dimensions[1] - 1) * upscale_y + a_y + weights->dimensions[1] - 2 * pad_y);
    }

    enum TensorCFmt fmt = getTensorCFmt(input);
    assert (fmt == getTensorCFmt(weights));
    assert (!biases || fmt == getTensorCFmt(biases));
    assert (fmt == getTensorCFmt(output));

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = calloc(out_td.dims[out_td.dim_num - 1],
            out_td.strides[out_td.dim_num - 1]);
    UNLESS (output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#else
    void * output_ptr = out->addr;
#endif

    DeconvolutionKernelImpl(
            fmt,
            input->addr, in_td,
            weights->addr, weight_td,
            (biases ? biases->addr : NULL), bias_td,
            pad_x, pad_y,
            upscale_x, upscale_y,
            overflow == VX_CONVERT_POLICY_WRAP,
            rounding == VX_ROUND_POLICY_TO_NEAREST_EVEN,
            a_x, a_y,
            output_ptr, out_td);

#ifdef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    const vx_size view_start[VX_MAX_TENSOR_DIMENSIONS] = { 0 };
    vx_status status = vxCopyTensorPatch(output, out_td.dim_num, view_start, out_td.dims,
            out_td.strides, output_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    free(output_ptr);

    return status;
#else
    return VX_SUCCESS;
#endif
}

static vx_status VX_CALLBACK nnDeconvolutionValidator(
        vx_node node,
        const vx_reference parameters[],
        vx_uint32 num,
        vx_meta_format metas[])
{
    (void)node;
    UNLESS (num == DECONV_PARAMS_NUMBER) return VX_ERROR_INVALID_PARAMETERS;

    vx_tensor input =       (vx_tensor)parameters[DECONV_PARAM_TENSOR_IN];
    vx_tensor weights =     (vx_tensor)parameters[DECONV_PARAM_WEIGHTS];
    vx_tensor biases =      (vx_tensor)parameters[DECONV_PARAM_BIASES];
    vx_scalar pad_x_sc =    (vx_scalar)parameters[DECONV_PARAM_PAD_X];
    vx_scalar pad_y_sc =    (vx_scalar)parameters[DECONV_PARAM_PAD_Y];
    vx_scalar overflow_sc = (vx_scalar)parameters[DECONV_PARAM_OVERFLOW];
    vx_scalar rounding_sc = (vx_scalar)parameters[DECONV_PARAM_ROUNDING];
    vx_scalar a_x_sc =      (vx_scalar)parameters[DECONV_PARAM_A_X];
    vx_scalar a_y_sc =      (vx_scalar)parameters[DECONV_PARAM_A_Y];
    vx_tensor output =      (vx_tensor)parameters[DECONV_PARAM_TENSOR_OUT];

    vx_size pad_x;
    vx_size pad_y;
    vx_enum overflow;
    vx_enum rounding;
    vx_size a_x;
    vx_size a_y;

    VX_CALL(scalarCheckGet(pad_x_sc, VX_TYPE_SIZE, &pad_x));
    VX_CALL(scalarCheckGet(pad_y_sc, VX_TYPE_SIZE, &pad_y));
    VX_CALL(scalarCheckGet(overflow_sc, VX_TYPE_ENUM, &overflow));
    VX_CALL(scalarCheckGet(rounding_sc, VX_TYPE_ENUM, &rounding));
    VX_CALL(scalarCheckGet(a_x_sc, VX_TYPE_SIZE, &a_x));
    VX_CALL(scalarCheckGet(a_y_sc, VX_TYPE_SIZE, &a_y));

    UNLESS (overflow == VX_CONVERT_POLICY_WRAP ||
            overflow == VX_CONVERT_POLICY_SATURATE) 
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer overflow param must be either VX_CONVERT_POLICY_WRAP or VX_CONVERT_POLICY_SATURATE"
                /* (either %d or %d, but got %d instead), VX_CONVERT_POLICY_WRAP, VX_CONVERT_POLICY_SATURATE, overflow*/);
        return VX_ERROR_INVALID_PARAMETERS;
    }

    UNLESS(rounding == VX_ROUND_POLICY_TO_ZERO ||
           rounding == VX_ROUND_POLICY_TO_NEAREST_EVEN) 
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer round param must be either VX_ROUND_POLICY_TO_ZERO or VX_ROUND_POLICY_TO_NEAREST_EVEN"
                /* (either %d or %d, but got %d instead), VX_ROUND_POLICY_TO_ZERO, VX_ROUND_POLICY_TO_NEAREST_EVEN, rounding*/);
        return VX_ERROR_INVALID_PARAMETERS;
    }

    //TODO: what's the proper way to use an internal API here?

    vx_enum i_dt, w_dt, b_dt, o_dt;
    VX_CALL(vxQueryTensor(input, VX_TENSOR_DATA_TYPE, &i_dt, sizeof(i_dt)));
    VX_CALL(vxQueryTensor(weights, VX_TENSOR_DATA_TYPE, &w_dt, sizeof(w_dt)));
    if (biases)
    {
        VX_CALL(vxQueryTensor(biases, VX_TENSOR_DATA_TYPE, &b_dt, sizeof(b_dt)));
    }
    VX_CALL(vxQueryTensor(output, VX_TENSOR_DATA_TYPE, &o_dt, sizeof(o_dt)));

    vx_uint8 i_fpp, w_fpp, b_fpp, o_fpp;
    VX_CALL(vxQueryTensor(input, VX_TENSOR_FIXED_POINT_POSITION, &i_fpp, sizeof(i_fpp)));
    VX_CALL(vxQueryTensor(weights, VX_TENSOR_FIXED_POINT_POSITION, &w_fpp, sizeof(w_fpp)));
    if (biases)
    {
        VX_CALL(vxQueryTensor(biases, VX_TENSOR_FIXED_POINT_POSITION, &b_fpp, sizeof(b_fpp)));
    }
    VX_CALL(vxQueryTensor(output, VX_TENSOR_FIXED_POINT_POSITION, &o_fpp, sizeof(o_fpp)));

    UNLESS ((o_dt == VX_TYPE_INT16 && o_fpp == Q78_FIXED_POINT_POSITION) ||
            (o_dt == VX_TYPE_INT8 && !o_fpp) ||
            (o_dt == VX_TYPE_UINT8 && !o_fpp))
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer only supports Q78, U8 and S8 formats");
        return VX_ERROR_INVALID_FORMAT;
    }

    UNLESS (i_dt == w_dt && (!biases || i_dt == b_dt) && i_dt == o_dt)
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires matching tensor data_types");
        return VX_ERROR_INVALID_FORMAT;
    }

    UNLESS (i_fpp == w_fpp && (!biases || i_fpp == b_fpp) && i_fpp == o_fpp)
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires matching tensor fixed_point_position");
        return VX_ERROR_INVALID_FORMAT;
    }

    tensor_desc_t in_td = getTensorDesc(input);
    tensor_desc_t weight_td = getTensorDesc(weights);
    tensor_desc_t bias_td = getOptionalTensorDesc(biases);
    tensor_desc_t out_td = getTensorDesc(output);

    UNLESS (in_td.dim_num == out_td.dim_num &&
            (in_td.dim_num == 3 || in_td.dim_num == 4))
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires 3 dimensionsal inputs and outputs with an additional optional batching dim");
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (in_td.dim_num == 3 || (in_td.dims[3] == out_td.dims[3]))
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires the batching dim to match between the input and output");
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (weight_td.dim_num == 4)
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires 4 dimensional weights");
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (weight_td.dims[2] == in_td.dims[2] && weight_td.dims[3] == out_td.dims[2])
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layers weights 3rd and 4th dim must match the IFM and OFM (3rd dim of input and output resp.)");
        return VX_ERROR_INVALID_DIMENSION;
    }

    // Note effectively our transposed conv padding is weight_sz - pad_sz - 1,
    // where pad_sz is the user provided "regular" conv padding.
    UNLESS (weight_td.dims[0] >= pad_x + 1 &&
            weight_td.dims[1] >= pad_y + 1)
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer should satisfy (weight_sz >= pad_sz + 1)");
        return VX_ERROR_INVALID_DIMENSION;
    }

    UNLESS (bias_td.dim_num == 0 ||
            (bias_td.dim_num == 1 && bias_td.dims[0] == out_td.dims[2]) ||
            (bias_td.dim_num == 3 && bias_td.dims[0] == out_td.dims[0] &&
             bias_td.dims[1] == out_td.dims[1] && bias_td.dims[2] == out_td.dims[2]))
    {
        VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer supports no bias, [OFM] and [Out W, Out H, OFM] bias layouts");
        return VX_ERROR_INVALID_DIMENSION;
    }

    // The following deals with upscale and dimensional relations
    {
        //TODO: verify these! we could infact allow the first two coniditons and use an upsacle of 1, just no time atm
        UNLESS (out_td.dims[0] + 2 * pad_x >= weight_td.dims[0] + a_x &&
                out_td.dims[1] + 2 * pad_y >= weight_td.dims[1] + a_y)
        {
            VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires (out_sz + 2 * pad_sz >= weight_sz + a) for upscale to be >= 1");
            return VX_ERROR_NOT_SUPPORTED;
        }

        UNLESS (in_td.dims[0] > 1 && in_td.dims[1] > 1)
        {
            VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires (input_sz > 1)");
            return VX_ERROR_NOT_SUPPORTED;
        }

        vx_size upscale_x = (out_td.dims[0] + 2 * pad_x - weight_td.dims[0] - a_x) / (in_td.dims[0] - 1);
        vx_size upscale_y = (out_td.dims[1] + 2 * pad_y - weight_td.dims[1] - a_y) / (in_td.dims[1] - 1);

        UNLESS (out_td.dims[0] == (in_td.dims[0] - 1) * upscale_x + a_x + weight_td.dims[0] - 2 * pad_x &&
                out_td.dims[1] == (in_td.dims[1] - 1) * upscale_y + a_y + weight_td.dims[1] - 2 * pad_y)
        {
            VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires (out_sz == (in_sz - 1) * upscale + a + weight_sz - 2 * pad_sz) to be satisfiable for an int upscale > 0");
            return VX_ERROR_INVALID_DIMENSION;
        }

        UNLESS (a_x >= 0 && a_x < upscale_x &&
                a_y >= 0 && a_y < upscale_y)
        {
            VX_PRINT(VX_ZONE_ERROR, "Deconvolution layer requires 0 <= a < upscale");
            return VX_ERROR_INVALID_DIMENSION;
        }
    }

    vx_meta_format * meta = &metas[DECONV_PARAM_TENSOR_OUT];
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &out_td.dim_num, sizeof(out_td.dim_num)));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_td.dims, sizeof(*out_td.dims) * out_td.dim_num));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &o_dt, sizeof(o_dt)));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &o_fpp, sizeof(o_fpp)));

    return VX_SUCCESS;
}

static vx_param_description_t nn_deconvolution_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_OPTIONAL },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

vx_kernel_description_t nn_deconvolution_kernel = {
    VX_KERNEL_DECONVOLUTION_LAYER,
    "org.khronos.nn_extension.deconvolution_layer",
    nnDeconvolutionKernel,
    nn_deconvolution_kernel_params, dimof(nn_deconvolution_kernel_params),
    nnDeconvolutionValidator,
    NULL, NULL, NULL, NULL,
};

#endif
#endif//OPENVX_CONFORMANCE_NEURAL_NETWORKS
