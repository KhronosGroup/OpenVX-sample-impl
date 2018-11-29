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
#include <VX/vx.h>
#include <VX/vx_helper.h>
#include "vx_internal.h"
#include <c_model.h>

#include <assert.h>


#define Q78_FIXED_POINT_POSITION 8

#define VX_CALL(expr_)   \
    do { \
        vx_status status_= (expr_); \
        if (status_ != VX_SUCCESS) return status_; \
    } while(0)


static enum TensorCFmt getTensorCFmt(vx_tensor tensor)
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

static vx_status tensorElementwiseBinaryMathOp(
        enum ElementwiseTensorMathOp op,
        vx_tensor in0,
        vx_tensor in1,
        vx_scalar scale_sc,     // optional, only !NULL for Mul
        vx_scalar overflow_sc,
        vx_scalar rounding_sc,  // optional, only !NULL for Mul
        vx_tensor out)
{
    float scale = 0.f;
    vx_enum overflow = 0;
    vx_enum rounding = 0;

    if (op == ELEMENTWISE_TENSOR_MUL)
    {
        VX_CALL(vxCopyScalar(scale_sc, &scale, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
        VX_CALL(vxCopyScalar(rounding_sc, &rounding, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    }
    else
    {
        assert (op == ELEMENTWISE_TENSOR_ADD || op == ELEMENTWISE_TENSOR_SUB);
    }

    VX_CALL(vxCopyScalar(overflow_sc, &overflow, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    tensor_desc_t in0_td = { in0->number_of_dimensions, in0->dimensions, in0->stride };
    tensor_desc_t in1_td = { in1->number_of_dimensions, in1->dimensions, in1->stride };
    tensor_desc_t out_td = { out->number_of_dimensions, out->dimensions, out->stride };

    const enum TensorCFmt fmt = getTensorCFmt(in0);
    assert (getTensorCFmt(in1) == fmt);
    assert (getTensorCFmt(out) == fmt);

#ifndef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = out->addr;
#else
    void * output_ptr = calloc(out_td.dims[out_td.dim_num - 1],
            out_td.strides[out_td.dim_num - 1]);
    if (!output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }

#endif
    ElementwiseTensorOpImpl(
            op,
            fmt,
            in0->addr, in0_td,
            in1->addr, in1_td,
            scale,
            overflow == VX_CONVERT_POLICY_WRAP,
            rounding == VX_ROUND_POLICY_TO_NEAREST_EVEN,
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



/*==============================================================================
==============================================================================
KERNELS AND INPUT/OUTPUT VALIDATORS
==============================================================================
=============================================================================*/
typedef enum _multiply_params_e {
    MULTIPLY_PARAM_TENSOR1 = 0,
    MULTIPLY_PARAM_TENSOR2,
    MULTIPLY_PARAM_SCALE,
    MULTIPLY_PARAM_OVERFLOW_POLICY,
    MULTIPLY_PARAM_ROUNDING_POLICY,
    MULTIPLY_PARAM_TENSOR_OUT,
    MULTIPLY_PARAMS_NUMBER
} multiply_params_e;



vx_status VX_CALLBACK tensorMultiplyKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == MULTIPLY_PARAMS_NUMBER)
    {
        vx_tensor in0 = (vx_tensor)parameters[MULTIPLY_PARAM_TENSOR1];
        vx_tensor in1 = (vx_tensor)parameters[MULTIPLY_PARAM_TENSOR2];
        vx_scalar scale_sc = (vx_scalar)parameters[MULTIPLY_PARAM_SCALE];
        vx_scalar overflow_sc = (vx_scalar)parameters[MULTIPLY_PARAM_OVERFLOW_POLICY];
        vx_scalar rounding_sc = (vx_scalar)parameters[MULTIPLY_PARAM_ROUNDING_POLICY];
        vx_tensor out = (vx_tensor)parameters[MULTIPLY_PARAM_TENSOR_OUT];

        return tensorElementwiseBinaryMathOp(
                ELEMENTWISE_TENSOR_MUL,
                in0,
                in1,
                scale_sc,
                overflow_sc,
                rounding_sc,  
                out);
    }

    return VX_ERROR_INVALID_PARAMETERS;
}



static vx_status validateOverflowPolicy (vx_scalar overflow_policy_sc)
{
    vx_status status = VX_SUCCESS;
    if (overflow_policy_sc)        /* overflow_policy: truncate or saturate. */
    {
        vx_enum stype = 0;
        vxQueryScalar(overflow_policy_sc, VX_SCALAR_TYPE, &stype, sizeof(stype));
        if (stype == VX_TYPE_ENUM)
        {
            vx_enum overflow_policy = 0;
            vxCopyScalar(overflow_policy_sc, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            if (overflow_policy == VX_CONVERT_POLICY_SATURATE || overflow_policy == VX_CONVERT_POLICY_WRAP)
            {
                status = VX_SUCCESS;
            }
            else
            {
                status = VX_ERROR_INVALID_VALUE;
            }
        }
        else
        {
            status = VX_ERROR_INVALID_TYPE;
        }
    } else {
        status = VX_ERROR_INVALID_REFERENCE;
    }

    return status;

}

static vx_status validateFloatScalar(vx_scalar sc, /*OUT*/ vx_float32 * f)
{
    if (! sc) return VX_ERROR_INVALID_REFERENCE;    //TODO: won't vxQueryScalar handle this anyway??

    vx_enum type = -1;
    VX_CALL(vxQueryScalar(sc, VX_SCALAR_TYPE, &type, sizeof(type)));
    if (type != VX_TYPE_FLOAT32) return VX_ERROR_INVALID_TYPE;

    // Use a temp, to avoid writting to the output (f) on failure
    vx_float32 temp;
    VX_CALL(vxCopyScalar(sc, &temp, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    if (f) *f = temp;

    return VX_SUCCESS;
}

static vx_status validateScaleFactor(vx_scalar scale_factor)
{
    vx_float32 tmp;

    VX_CALL(validateFloatScalar(scale_factor, &tmp));

    return tmp >= 0 ? VX_SUCCESS : VX_ERROR_INVALID_VALUE;
}

static vx_status validateRoundingPolicy (vx_scalar rounding_policy_sc)
{
    vx_status status = VX_SUCCESS;
    if (rounding_policy_sc)        /* rounding_policy: truncate or saturate. */
    {
            vx_enum stype = 0;
            vxQueryScalar(rounding_policy_sc, VX_SCALAR_TYPE, &stype, sizeof(stype));
            if (stype == VX_TYPE_ENUM)
            {
                vx_enum rouding_policy = 0;
                vxCopyScalar(rounding_policy_sc, &rouding_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                if ((rouding_policy == VX_ROUND_POLICY_TO_ZERO) ||
                    (rouding_policy == VX_ROUND_POLICY_TO_NEAREST_EVEN))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_VALUE;
                }
            }
            else
            {
                status = VX_ERROR_INVALID_TYPE;
            }
    }  else {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;

}

static vx_status CalculateOutputDimensions (vx_size* in1_dim, vx_size* in2_dim, vx_size* out_dim_calculated, vx_size dims_num)
{
    vx_status status = VX_SUCCESS;

    for (int i = 0; i < (int)dims_num; i++)
    {
        if (in1_dim[i] == in2_dim[i])
        {
            out_dim_calculated[i] = in1_dim[i];
        }
        else if (in1_dim[i] == 1 || in2_dim[i] == 1)    //TODO: what do we reall want to do here? 
        {
            out_dim_calculated[i] = in2_dim[i] > in1_dim[i] ? in2_dim[i] : in1_dim[i];
        }
        else
            status = VX_ERROR_INVALID_DIMENSION;
    }
    return status;

}
static vx_status SetOutputMetaFormat (vx_tensor in1, vx_tensor in2, vx_tensor out, vx_meta_format * meta)
{
    vx_status status = VX_SUCCESS;
    vx_size in1_dim[VX_MAX_TENSOR_DIMENSIONS];
    vx_size in2_dim[VX_MAX_TENSOR_DIMENSIONS];
    vx_size out_dim[VX_MAX_TENSOR_DIMENSIONS];
    vx_size out_dim_calculated[VX_MAX_TENSOR_DIMENSIONS];
    vx_size in1_num_dims, in2_num_dims, out_num_dims;
    vx_enum format1, format2, format_out;
    vx_uint8 fixed_point_pos1 = 0, fixed_point_pos2 = 0, fixed_point_pos_out = 0;

    vxQueryTensor(in1, VX_TENSOR_DATA_TYPE, &format1, sizeof(format1));
    vxQueryTensor(in1, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos1, sizeof(fixed_point_pos1));

    vxQueryTensor(in2, VX_TENSOR_DATA_TYPE, &format2, sizeof(format2));
    vxQueryTensor(in2, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos2, sizeof(fixed_point_pos2));

    vxQueryTensor(out, VX_TENSOR_DATA_TYPE, &format_out, sizeof(format_out));
    vxQueryTensor(out, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos_out, sizeof(fixed_point_pos_out));

    if ((format1 != format2) || (fixed_point_pos1 != fixed_point_pos2) ||
            (format_out != VX_TYPE_INVALID && format_out != format1) ||
            (fixed_point_pos_out != 0 && fixed_point_pos_out != fixed_point_pos1))
    {
        status = VX_ERROR_INVALID_FORMAT;
    } else {
        status |= vxQueryTensor(in1, VX_TENSOR_DIMS, in1_dim, sizeof (in1_dim));
        status |= vxQueryTensor(in2, VX_TENSOR_DIMS, in2_dim, sizeof (in2_dim));
        status |= vxQueryTensor(out, VX_TENSOR_DIMS, out_dim, sizeof (out_dim));

        status |= vxQueryTensor(in1, VX_TENSOR_NUMBER_OF_DIMS, &in1_num_dims, sizeof(in1_num_dims));
        status |= vxQueryTensor(in2, VX_TENSOR_NUMBER_OF_DIMS, &in2_num_dims, sizeof(in2_num_dims));
        status |= vxQueryTensor(out, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
        if (in1_num_dims != in2_num_dims || (out_num_dims != 0 && out_num_dims != in1_num_dims))
        {
            status = VX_ERROR_INVALID_DIMENSION;
        } else {
            status = CalculateOutputDimensions (in1_dim, in2_dim, out_dim_calculated, in1_num_dims);
            if (status == VX_SUCCESS)
            {
                for (int i = 0; i < (int)out_num_dims; i++)
                {
                    if (out_dim[i] != 0 && out_dim[i] != out_dim_calculated[i])
                    {
                        status = VX_ERROR_INVALID_DIMENSION;
                        break;
                    }
                }
            }
            if (status == VX_SUCCESS)
            {
                status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &format1, sizeof(format1));
                status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos1, sizeof(fixed_point_pos1));
                status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_dim_calculated, sizeof(out_dim_calculated));
                status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &in1_num_dims, sizeof(in1_num_dims));
            }
        }
    }


    return status;
}
/*! \brief TensorMultiply parameters validator.
 * \param [in] node The handle to the node.
 * \param [in] parameters The array of parameters to be validated.
 * \param [in] num Number of parameters to be validated.
 * \param [out] metas The array of metadata used to check output parameters only.
 * \return A \ref vx_status_e enumeration.
 * \ingroup group_example_kernel
 */
static vx_status VX_CALLBACK tensorMultiplyValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;

    if (num != MULTIPLY_PARAMS_NUMBER)
        return VX_ERROR_INVALID_PARAMETERS;
    // Check input tensors
    vx_tensor in1 = (vx_tensor)parameters[MULTIPLY_PARAM_TENSOR1];
    vx_tensor in2 = (vx_tensor)parameters[MULTIPLY_PARAM_TENSOR2];
    vx_tensor out = (vx_tensor)parameters[MULTIPLY_PARAM_TENSOR_OUT];
    vx_status status = VX_SUCCESS;
    status = validateOverflowPolicy((vx_scalar)parameters[MULTIPLY_PARAM_OVERFLOW_POLICY]);
    status |= validateRoundingPolicy((vx_scalar)parameters[MULTIPLY_PARAM_ROUNDING_POLICY]);
    status |= validateScaleFactor((vx_scalar)parameters[MULTIPLY_PARAM_SCALE]);
    status |= SetOutputMetaFormat (in1, in2, out, &metas[MULTIPLY_PARAM_TENSOR_OUT]);


    return VX_SUCCESS;
}

static vx_param_description_t tensor_multiply_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t tensor_multiply_kernel = {
    VX_KERNEL_TENSOR_MULTIPLY,
    "org.khronos.openvx.tensor_multiply",
    tensorMultiplyKernel,
	tensor_multiply_kernel_params, dimof(tensor_multiply_kernel_params),
	tensorMultiplyValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};

typedef enum _addsub_params_e {
    ADDSUB_PARAM_TENSOR1 = 0,
    ADDSUB_PARAM_TENSOR2,
    ADDSUB_PARAM_OVERFLOW_POLICY,
    ADDSUB_PARAM_TENSOR_OUT,
    ADDSUB_PARAMS_NUMBER
} addsub_params_e;


static vx_status VX_CALLBACK tensorAddKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == ADDSUB_PARAMS_NUMBER)
    {
        vx_tensor in0 = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR1];
        vx_tensor in1 = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR2];
        vx_scalar overflow_sc = (vx_scalar)parameters[ADDSUB_PARAM_OVERFLOW_POLICY];
        vx_tensor out = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR_OUT];

        return tensorElementwiseBinaryMathOp(
                ELEMENTWISE_TENSOR_ADD,
                in0,
                in1,
                NULL,
                overflow_sc,
                NULL,
                out);
    }

    return VX_ERROR_INVALID_PARAMETERS;
}
/*! \brief Add/Subtract parameters validator.
 * \param [in] node The handle to the node.
 * \param [in] parameters The array of parameters to be validated.
 * \param [in] num Number of parameters to be validated.
 * \param [out] metas The array of metadata used to check output parameters only.
 * \return A \ref vx_status_e enumeration.
 * \ingroup group_example_kernel
 */
static vx_status VX_CALLBACK tensorAddSubValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;

    if (num != ADDSUB_PARAMS_NUMBER)
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    // Check input tensors
    vx_tensor in1 = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR1];
    vx_tensor in2 = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR2];
    vx_tensor out = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR_OUT];
    vx_status status = VX_SUCCESS;
    status = validateOverflowPolicy((vx_scalar)parameters[ADDSUB_PARAM_OVERFLOW_POLICY]);
    status |= SetOutputMetaFormat (in1, in2, out, &metas[ADDSUB_PARAM_TENSOR_OUT]);


    return VX_SUCCESS;
}



static vx_param_description_t tensor_addsub_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t tensor_add_kernel = {
    VX_KERNEL_TENSOR_ADD,
    "org.khronos.openvx.tensor_add",
    tensorAddKernel,
    tensor_addsub_kernel_params, dimof(tensor_addsub_kernel_params),
    tensorAddSubValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};

static vx_status VX_CALLBACK tensorSubtractKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == ADDSUB_PARAMS_NUMBER)
    {
        vx_tensor in0 = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR1];
        vx_tensor in1 = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR2];
        vx_scalar overflow_sc = (vx_scalar)parameters[ADDSUB_PARAM_OVERFLOW_POLICY];
        vx_tensor out = (vx_tensor)parameters[ADDSUB_PARAM_TENSOR_OUT];

        return tensorElementwiseBinaryMathOp(
                ELEMENTWISE_TENSOR_SUB,
                in0,
                in1,
                NULL,
                overflow_sc,
                NULL,
                out);
    }

    return VX_ERROR_INVALID_PARAMETERS;
}




vx_kernel_description_t tensor_subtract_kernel = {
    VX_KERNEL_TENSOR_SUBTRACT,
    "org.khronos.openvx.tensor_subtract",
    tensorSubtractKernel,
    tensor_addsub_kernel_params, dimof(tensor_addsub_kernel_params),
    tensorAddSubValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};

/****************************************************************************
 *                                                                          *
 *                          vxTensorConvertDepthNode                        *
 *                                                                          *
 ***************************************************************************/

enum tensor_convert_depth_params_e {
    CONVERT_DEPTH_PARAM_IN,
    CONVERT_DEPTH_PARAM_OVERFLOW,
    CONVERT_DEPTH_PARAM_NORM,
    CONVERT_DEPTH_PARAM_OFFSET,
    CONVERT_DEPTH_PARAM_OUT,
    CONVERT_DEPTH_PARAM_COUNT,
};

static vx_status VX_CALLBACK tensorConvertDepthKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num != CONVERT_DEPTH_PARAM_COUNT)
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    vx_tensor in          = (vx_tensor)parameters[CONVERT_DEPTH_PARAM_IN];
    vx_scalar overflow_sc = (vx_scalar)parameters[CONVERT_DEPTH_PARAM_OVERFLOW];
    vx_scalar norm_sc     = (vx_scalar)parameters[CONVERT_DEPTH_PARAM_NORM];
    vx_scalar offset_sc   = (vx_scalar)parameters[CONVERT_DEPTH_PARAM_OFFSET];
    vx_tensor out         = (vx_tensor)parameters[CONVERT_DEPTH_PARAM_OUT];

    enum vx_convert_policy_e overflow;
    vx_float32 norm;
    vx_float32 offset;

    VX_CALL(vxCopyScalar(overflow_sc, &overflow, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(norm_sc, &norm, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));
    VX_CALL(vxCopyScalar(offset_sc, &offset, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

    tensor_desc_t in_td = { in->number_of_dimensions, in->dimensions, in->stride };
    tensor_desc_t out_td = { out->number_of_dimensions, out->dimensions, out->stride };

    const enum TensorCFmt src_fmt = getTensorCFmt(in);
    const enum TensorCFmt dst_fmt = getTensorCFmt(out);

#ifndef HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC
    void * output_ptr = out->addr;
#else
    void * output_ptr = calloc(out_td.dims[out_td.dim_num - 1],
            out_td.strides[out_td.dim_num - 1]);
    if (!output_ptr)
    {
        return VX_ERROR_NO_MEMORY;
    }
#endif

    TensorConvertDepthKernelImpl(
            in->addr, in_td,
            src_fmt,
            dst_fmt,
            overflow == VX_CONVERT_POLICY_WRAP,
            norm,
            offset,
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

static vx_status VX_CALLBACK tensorConvertDepthValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;

    if (num != CONVERT_DEPTH_PARAM_COUNT)
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    vx_tensor in          = (vx_tensor)parameters[CONVERT_DEPTH_PARAM_IN];
    vx_scalar overflow_sc = (vx_scalar)parameters[CONVERT_DEPTH_PARAM_OVERFLOW];
    vx_scalar norm_sc     = (vx_scalar)parameters[CONVERT_DEPTH_PARAM_NORM];
    vx_scalar offset_sc   = (vx_scalar)parameters[CONVERT_DEPTH_PARAM_OFFSET];
    vx_tensor out         = (vx_tensor)parameters[CONVERT_DEPTH_PARAM_OUT];

    VX_CALL(validateOverflowPolicy(overflow_sc));
    VX_CALL(validateFloatScalar(norm_sc, NULL));
    VX_CALL(validateFloatScalar(offset_sc, NULL));

    if (!in || !out)
    {
        //TODO: err msg
        return VX_ERROR_INVALID_PARAMETERS;
    }

    //TODO: We probably should not be using external APIs here (?)

    vx_enum in_fmt, out_fmt;
    vx_uint8 in_fixed_point_pos, out_fixed_point_pos;

    //TODO: should use a VX_CALL_ERR with custom msg
    VX_CALL(vxQueryTensor(in, VX_TENSOR_DATA_TYPE, &in_fmt, sizeof(in_fmt)));
    VX_CALL(vxQueryTensor(in, VX_TENSOR_FIXED_POINT_POSITION, &in_fixed_point_pos, sizeof(in_fixed_point_pos)));

    VX_CALL(vxQueryTensor(out, VX_TENSOR_DATA_TYPE, &out_fmt, sizeof(out_fmt)));
    VX_CALL(vxQueryTensor(out, VX_TENSOR_FIXED_POINT_POSITION, &out_fixed_point_pos, sizeof(out_fixed_point_pos)));

    //TODO: use ownIsValidFormat(...)
    const bool valid_in_fmt =
        (in_fmt == VX_TYPE_INT16 && in_fixed_point_pos == Q78_FIXED_POINT_POSITION) ||
        ((in_fmt == VX_TYPE_UINT8 || in_fmt == VX_TYPE_INT8) && !in_fixed_point_pos);

    const bool valid_out_fmt =
        (out_fmt == VX_TYPE_INT16 && out_fixed_point_pos == Q78_FIXED_POINT_POSITION) ||
        ((out_fmt == VX_TYPE_UINT8 || out_fmt == VX_TYPE_INT8) && !out_fixed_point_pos);

    if (!valid_in_fmt || !valid_out_fmt)
    {
        //TODO: err msg
        return VX_ERROR_INVALID_FORMAT;
    }

    vx_size out_dim_num = out->number_of_dimensions;
    vx_size * out_dims = out->dimensions;

    if (out_dim_num != in->number_of_dimensions)
    {
        //TODO: err msg
        return VX_ERROR_INVALID_DIMENSION;
    }

    for (vx_size i = 0; i < out_dim_num; ++i)
    {
        if (out_dims[i] != in->dimensions[i])
        {
            //TODO: err msg
            return VX_ERROR_INVALID_DIMENSION;
        }
    }

    vx_meta_format * const meta = &metas[CONVERT_DEPTH_PARAM_OUT];
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &out_fmt, sizeof(out_fmt)));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &out_fixed_point_pos, sizeof(out_fixed_point_pos)));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_dims, sizeof(*out_dims) * out_dim_num));
    VX_CALL(vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &out_dim_num, sizeof(out_dim_num)));

    return VX_SUCCESS;
}

static vx_param_description_t tensor_convert_depth_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

vx_kernel_description_t tensor_convert_depth_kernel = {
    VX_KERNEL_TENSOR_CONVERT_DEPTH,
    "org.khronos.openvx.tensor_convert_depth",
    tensorConvertDepthKernel,
    tensor_convert_depth_kernel_params, dimof(tensor_convert_depth_kernel_params),
    tensorConvertDepthValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};
