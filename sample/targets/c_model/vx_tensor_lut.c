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

typedef enum _lut_params_e {
    LUT_PARAM_TENSOR_IN = 0,
    LUT_PARAM_LUT,
    LUT_PARAM_TENSOR_OUT,
    LUT_PARAMS_NUMBER
} lut_params_e;

static vx_param_description_t tensor_lut_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_LUT, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

static vx_status VX_CALLBACK tensorTableLookupKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    (void)node;

    if (num == LUT_PARAMS_NUMBER)
    {
        vx_tensor src_tensor =  (vx_tensor)parameters[LUT_PARAM_TENSOR_IN];
        vx_lut   lut       =    (vx_lut)parameters[LUT_PARAM_LUT];
        vx_tensor dst_tensor =  (vx_tensor)parameters[LUT_PARAM_TENSOR_OUT];
        vx_status status = VX_SUCCESS;
        void *lut_ptr = NULL;
        vx_size count = 0;
        vx_uint32 offset = 0;
        vx_size dims_num;
        vx_size dims[VX_MAX_TENSOR_DIMENSIONS];
        vx_size stride[VX_MAX_TENSOR_DIMENSIONS];
        void *out_ptr;

        status |= vxQueryLUT(lut, VX_LUT_COUNT, &count, sizeof(count));
        status |= vxQueryLUT(lut, VX_LUT_OFFSET, &offset, sizeof(offset));
        status |= vxAccessLUT(lut, &lut_ptr, VX_READ_ONLY);
        status |= AllocatePatch (dst_tensor, &dims_num, dims, stride, &out_ptr, VX_WRITE_ONLY);

        status |= vxTensorTableLookup(src_tensor->addr, src_tensor->stride, src_tensor->dimensions, src_tensor->number_of_dimensions, lut_ptr,
                count, offset, out_ptr, dst_tensor->stride, dst_tensor->data_type);

        status |= vxCommitLUT(lut, lut_ptr);
        status |= ReleasePatch(dst_tensor, dims_num, dims, stride, &out_ptr, VX_WRITE_ONLY);

        //return vxTensorTableLookup(src_tensor, lut, dst_tensor);
        return status;
    }

    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status validateInputs (vx_tensor tensor, vx_lut lut, vx_size out_dims[], vx_size* num_of_dims, vx_enum* format, vx_int8* fixed_point_pos)
{
    vx_status status = VX_SUCCESS;
    status |= vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE, format, sizeof(*format));
    status |= vxQueryTensor(tensor, VX_TENSOR_FIXED_POINT_POSITION, fixed_point_pos, sizeof(*fixed_point_pos));

    status |= vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, num_of_dims, sizeof(*num_of_dims));
    status |= vxQueryTensor(tensor, VX_TENSOR_DIMS, out_dims, sizeof (*out_dims) * *num_of_dims);
    vx_enum lut_type = 0;
    status |= vxQueryLUT(lut, VX_LUT_TYPE, &lut_type, sizeof(lut_type));

    if (status == VX_SUCCESS)
    {
        if (lut_type != VX_TYPE_INT16 && lut_type != VX_TYPE_UINT8)
        {
            status = VX_ERROR_INVALID_FORMAT;
        }
        else if ((*format != VX_TYPE_INT16 && *format != VX_TYPE_UINT8) ||
                (*fixed_point_pos != 0 && *fixed_point_pos != Q78_FIXED_POINT_POSITION) ||
                (*fixed_point_pos == Q78_FIXED_POINT_POSITION && *format != VX_TYPE_INT16) ||
                (*fixed_point_pos == 0 && *format != VX_TYPE_UINT8))
                {
                    status = VX_ERROR_INVALID_FORMAT;
                }
    }

    return status;
}

static vx_status setLUTOutputMetaFormat (vx_meta_format * meta, vx_size out_dims[], vx_size num_of_dims, vx_enum format, vx_int8 fixed_point_pos)
{
    vx_status status = VX_SUCCESS;
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_dims, num_of_dims*sizeof(vx_size));
    return status;

}

static vx_status VX_CALLBACK tensorTableLookupValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    (void)node;

    if (num != LUT_PARAMS_NUMBER)
        return VX_ERROR_INVALID_PARAMETERS;
    // Check input tensors
    vx_tensor in = (vx_tensor)parameters[LUT_PARAM_TENSOR_IN];
    vx_lut lut = (vx_lut)parameters[LUT_PARAM_LUT];
    vx_status status = VX_SUCCESS;
    vx_size out_dims[VX_MAX_TENSOR_DIMENSIONS];
    vx_int8 fixed_point_pos = 0;
    vx_enum format = VX_TYPE_INVALID;
    vx_size num_of_dims = 0;
    status = validateInputs (in, lut, out_dims, &num_of_dims, &format, &fixed_point_pos);

    status |= setLUTOutputMetaFormat (&metas[LUT_PARAM_TENSOR_OUT], out_dims, num_of_dims, format, fixed_point_pos);


    return status;
}


vx_kernel_description_t tensor_lut_kernel  = {
     VX_KERNEL_TENSOR_TABLE_LOOKUP,
    "org.khronos.openvx.tensor_table_lookup",
    tensorTableLookupKernel,
    tensor_lut_kernel_params, dimof(tensor_lut_kernel_params),
    tensorTableLookupValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};



