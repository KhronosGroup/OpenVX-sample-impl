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

#include <math.h>
#include "tensor_utils.h"

typedef enum _bilateral_filter_params_e {
    BILATERAL_FILTER_PARAM_SRC = 0,
    BILATERAL_FILTER_PARAM_DIAMETER,
    BILATERAL_FILTER_PARAM_SIGMASPACE,
    BILATERAL_FILTER_PARAM_SIGMAVALUES,
    BILATERAL_FILTER_PARAM_DST,
    BILATERAL_FILTER_PARAMS_NUMBER
} bilateral_filter_params_e;

static vx_param_description_t bilateral_filter_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

static vx_status VX_CALLBACK vxBilateralFilterKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == BILATERAL_FILTER_PARAMS_NUMBER)
    {
        vx_status status = VX_SUCCESS;
        vx_tensor src_tensor =  (vx_tensor)parameters[BILATERAL_FILTER_PARAM_SRC];
        vx_tensor dst_tensor =  (vx_tensor)parameters[BILATERAL_FILTER_PARAM_DST];

        vx_border_t bordermode;
        vx_int32 diameter;
        vx_float32 sigmaSpace, sigmaValues;
        status |= vxCopyScalar((vx_scalar)parameters[BILATERAL_FILTER_PARAM_DIAMETER], &diameter, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar((vx_scalar)parameters[BILATERAL_FILTER_PARAM_SIGMASPACE], &sigmaSpace, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar((vx_scalar)parameters[BILATERAL_FILTER_PARAM_SIGMAVALUES], &sigmaValues, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));

        vx_size dims_num;
        vx_size dims[VX_MAX_TENSOR_DIMENSIONS];
        vx_size stride[VX_MAX_TENSOR_DIMENSIONS];
        void *out_ptr;
        if (status == VX_SUCCESS)
        {

            status = AllocatePatch (dst_tensor, &dims_num, dims, stride, &out_ptr, VX_WRITE_ONLY);

            status |= vxBilateralFilter(src_tensor->addr, src_tensor->stride, src_tensor->dimensions, src_tensor->number_of_dimensions,
                    diameter, sigmaSpace, sigmaValues, out_ptr, dst_tensor->stride, dst_tensor->data_type,
                    &bordermode);

            status |= ReleasePatch(dst_tensor, dims_num, dims, stride, &out_ptr, VX_WRITE_ONLY);
        }

        return status;
    }

    return VX_ERROR_INVALID_PARAMETERS;
}

vx_status validateBLInputs (vx_tensor in, vx_int32 diameter, vx_float32 sigmaSpace, vx_float32 sigmaValues,
        vx_size out_dims[], vx_size* num_of_dims, vx_enum* format, vx_int8* fixed_point_pos)
{
    vx_status status = VX_SUCCESS;
    status |= vxQueryTensor(in, VX_TENSOR_DATA_TYPE, format, sizeof(*format));
    status |= vxQueryTensor(in, VX_TENSOR_FIXED_POINT_POSITION, fixed_point_pos, sizeof(*fixed_point_pos));
    status |= vxQueryTensor(in, VX_TENSOR_NUMBER_OF_DIMS, num_of_dims, sizeof(*num_of_dims));
    status |= vxQueryTensor(in, VX_TENSOR_DIMS, out_dims, sizeof (*out_dims) * *num_of_dims);

    if (status == VX_SUCCESS)
    {
        if(diameter <=3 || diameter >= 10 || diameter%2 == 0 )
        {
            status = VX_ERROR_INVALID_FORMAT;
        }

        if(sigmaSpace <= 0 || sigmaSpace > 20)
        {
            status = VX_ERROR_INVALID_FORMAT;
        }

        if(sigmaValues <= 0 || sigmaValues > 20)
        {
            status = VX_ERROR_INVALID_FORMAT;
        }

        if ((*format != VX_TYPE_INT16 && *format != VX_TYPE_UINT8) ||
            (*fixed_point_pos != 0 && *fixed_point_pos != Q78_FIXED_POINT_POSITION) ||
            (*fixed_point_pos == Q78_FIXED_POINT_POSITION && *format != VX_TYPE_INT16) ||
            (*fixed_point_pos == 0 && *format != VX_TYPE_UINT8))
        {
            status = VX_ERROR_INVALID_FORMAT;
        }
    }
    return status;
}

vx_status SetBLOutputMetaFormat (vx_meta_format * meta, vx_size out_dims[], vx_size num_of_dims, vx_enum format, vx_int8 fixed_point_pos)
{
    vx_status status = VX_SUCCESS;
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_dims, num_of_dims*sizeof(vx_size));
    return status;
}

static vx_status VX_CALLBACK vxBilateralFilterValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;

    if (num != BILATERAL_FILTER_PARAMS_NUMBER)
        return VX_ERROR_INVALID_PARAMETERS;

    // Check input tensors
    vx_status status = VX_SUCCESS;
    vx_tensor in = (vx_tensor)parameters[BILATERAL_FILTER_PARAM_SRC];

    vx_size out_dims[VX_MAX_TENSOR_DIMENSIONS];
    vx_int8 fixed_point_pos = 0;
    vx_enum format = VX_TYPE_INVALID;
    vx_size num_of_dims = 0;

    vx_int32 diameter;
    vx_float32 sigmaSpace, sigmaValues;
    status |= vxCopyScalar((vx_scalar)parameters[BILATERAL_FILTER_PARAM_DIAMETER], &diameter, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar((vx_scalar)parameters[BILATERAL_FILTER_PARAM_SIGMASPACE], &sigmaSpace, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar((vx_scalar)parameters[BILATERAL_FILTER_PARAM_SIGMAVALUES], &sigmaValues, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    status = validateBLInputs (in, diameter, sigmaSpace, sigmaValues, out_dims, &num_of_dims, &format, &fixed_point_pos);
    status |= SetBLOutputMetaFormat (&metas[BILATERAL_FILTER_PARAM_DST], out_dims, num_of_dims, format, fixed_point_pos);

    return status;
}


vx_kernel_description_t bilateral_filter_kernel  = {
     VX_KERNEL_BILATERAL_FILTER,
    "org.khronos.openvx.bilateral_filter",
    vxBilateralFilterKernel,
    bilateral_filter_kernel_params, dimof(bilateral_filter_kernel_params),
    vxBilateralFilterValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};



