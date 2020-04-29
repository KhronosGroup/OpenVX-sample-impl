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
#include <venum.h>

#include <math.h>
#include "tensor_utils.h"

typedef enum _matrix_multiply_params_e {
    MATRIX_MULTIPLY_PARAM_TENSOR_IN1 = 0,
    MATRIX_MULTIPLY_PARAM_TENSOR_IN2,
    MATRIX_MULTIPLY_PARAM_TENSOR_IN3,
    MATRIX_MULTIPLY_PARAM_TRANSPOSE1,
    MATRIX_MULTIPLY_PARAM_TRANSPOSE2,
    MATRIX_MULTIPLY_PARAM_TRANSPOSE3,
    MATRIX_MULTIPLY_PARAM_TENSOR_OUT,
    MATRIX_MULTIPLY_PARAMS_NUMBER
} matrix_multiply_params_e;

static vx_param_description_t tensor_matrix_multiply_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_OPTIONAL },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },  //TODO: should this be optional as well in pair with in3?
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};

static inline void matrixTranspose(
        vx_tensor tensor, vx_bool transpose,
        /*OUT*/ vx_size* dims, /*OUT*/ vx_size* strides)
{
    dims[0] = transpose? tensor->dimensions[1]:tensor->dimensions[0];
    dims[1] = transpose? tensor->dimensions[0]:tensor->dimensions[1];
    strides[0] = transpose? tensor->stride[1]:tensor->stride[0];
    strides[1] = transpose? tensor->stride[0]:tensor->stride[1];

}

static vx_status VX_CALLBACK tensorMultiplyMatrixKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == MATRIX_MULTIPLY_PARAMS_NUMBER)
    {
        vx_tensor src1_tensor =  (vx_tensor)parameters[MATRIX_MULTIPLY_PARAM_TENSOR_IN1];
        vx_tensor src2_tensor =  (vx_tensor)parameters[MATRIX_MULTIPLY_PARAM_TENSOR_IN2];
        vx_tensor src3_tensor =  (vx_tensor)parameters[MATRIX_MULTIPLY_PARAM_TENSOR_IN3];
        vx_bool transpose1, transpose2, transpose3;
        vx_size dim1[2], dim2[2], dim3[2],strides1[2], strides2[2], strides3[2];

        vx_tensor dst_tensor =  (vx_tensor)parameters[MATRIX_MULTIPLY_PARAM_TENSOR_OUT];
        vx_status status = VX_SUCCESS;
        status |= vxCopyScalar((vx_scalar)parameters[MATRIX_MULTIPLY_PARAM_TRANSPOSE1], &transpose1, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar((vx_scalar)parameters[MATRIX_MULTIPLY_PARAM_TRANSPOSE2], &transpose2, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
        status |= vxCopyScalar((vx_scalar)parameters[MATRIX_MULTIPLY_PARAM_TRANSPOSE3], &transpose3, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

        matrixTranspose (src1_tensor, transpose1, dim1, strides1);
        matrixTranspose (src2_tensor, transpose2, dim2, strides2);
        if (src3_tensor)
        {
            matrixTranspose (src3_tensor, transpose3, dim3, strides3);
        }

        vx_size out_dim_num;
        vx_size out_dims[VX_MAX_TENSOR_DIMENSIONS];
        vx_size out_strides[VX_MAX_TENSOR_DIMENSIONS];
        void *out_ptr;

        status |= AllocatePatch (dst_tensor, &out_dim_num, out_dims, out_strides, &out_ptr, VX_WRITE_ONLY);

        Multiply2DMatrixesImpl(
                src1_tensor->addr, strides1, dim1,
                src2_tensor->addr, strides2, dim2,
                src3_tensor ? src3_tensor->addr : NULL, strides3,
                out_ptr, out_strides,
                dst_tensor->data_type);

        status |= ReleasePatch(dst_tensor, out_dim_num, out_dims, out_strides, &out_ptr, VX_WRITE_ONLY);

        return status;
    }

    return VX_ERROR_INVALID_PARAMETERS;
}

vx_status validateMMInputs (
        vx_tensor in1, vx_bool transpose1,
        vx_tensor in2, vx_bool transpose2,
        vx_tensor in3, vx_bool transpose3,
        /*OUT*/ vx_size* out_dims,
        /*OUT*/ vx_size* num_of_dims, /*OUT*/ vx_enum* format, /*OUT*/ vx_int8* fixed_point_pos)
{
    vx_status status = VX_SUCCESS;

    status |= vxQueryTensor(in1, VX_TENSOR_DATA_TYPE, format, sizeof(*format));
    status |= vxQueryTensor(in1, VX_TENSOR_FIXED_POINT_POSITION, fixed_point_pos, sizeof(*fixed_point_pos));
    status |= vxQueryTensor(in1, VX_TENSOR_NUMBER_OF_DIMS, num_of_dims, sizeof(*num_of_dims));

    vx_size dim1[2], dim2[2], dim3[2],strides1[2], strides2[2], strides3[2];
    matrixTranspose (in1, transpose1, dim1, strides1);
    matrixTranspose (in2, transpose2, dim2, strides2);
    if (in3)
    {
        matrixTranspose (in3, transpose3, dim3, strides3);
    }
    else
    {
        dim3[0] = 0;
        dim3[1] = 1;
    }

    if (status == VX_SUCCESS)
    {
        if (*format != VX_TYPE_INT16 && *format != VX_TYPE_UINT8 && *format != VX_TYPE_INT8)
        {
            status = VX_ERROR_INVALID_FORMAT;
        }
        else if ((*fixed_point_pos != 0 && *fixed_point_pos != Q78_FIXED_POINT_POSITION) ||
                (*fixed_point_pos == Q78_FIXED_POINT_POSITION && *format != VX_TYPE_INT16) ||
                (*fixed_point_pos == 0 && *format == VX_TYPE_INT16))
                {
                    status = VX_ERROR_INVALID_FORMAT;
                }
        else if (dim1[0] != dim2[1] || in1->number_of_dimensions != 2 || in2->number_of_dimensions != 2 ||
                 (in3 && (dim1[1] != dim3[1] || dim2[0] != dim3[0] || in3->number_of_dimensions != 2)))
        {
            status = VX_ERROR_INVALID_DIMENSION;
        }
        out_dims[0] = dim2[0];
        out_dims[1] = dim1[1];
    }

    return status;
}

vx_status SetMMOutputMetaFormat (vx_meta_format * meta, vx_size out_dims[], vx_size num_of_dims, vx_enum format, vx_int8 fixed_point_pos)
{
    vx_status status = VX_SUCCESS;
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &num_of_dims, sizeof(num_of_dims));
    status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_dims, num_of_dims*sizeof(vx_size));
    return status;

}

static vx_status VX_CALLBACK tensorMatrixMultiplyValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    if (num != MATRIX_MULTIPLY_PARAMS_NUMBER)
        return VX_ERROR_INVALID_PARAMETERS;
    // Check input tensors
    vx_tensor in1 = (vx_tensor)parameters[MATRIX_MULTIPLY_PARAM_TENSOR_IN1];
    vx_tensor in2 = (vx_tensor)parameters[MATRIX_MULTIPLY_PARAM_TENSOR_IN2];
    vx_tensor in3 = (vx_tensor)parameters[MATRIX_MULTIPLY_PARAM_TENSOR_IN3];
    vx_status status = VX_SUCCESS;
    vx_size out_dims[VX_MAX_TENSOR_DIMENSIONS];
    vx_int8 fixed_point_pos = 0;
    vx_enum format = VX_TYPE_INVALID;
    vx_size num_of_dims = 0;
    vx_bool transpose1, transpose2, transpose3;
    status |= vxCopyScalar((vx_scalar)parameters[MATRIX_MULTIPLY_PARAM_TRANSPOSE1], &transpose1, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar((vx_scalar)parameters[MATRIX_MULTIPLY_PARAM_TRANSPOSE2], &transpose2, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar((vx_scalar)parameters[MATRIX_MULTIPLY_PARAM_TRANSPOSE3], &transpose3, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status = validateMMInputs(in1, transpose1, in2, transpose2, in3, transpose3, out_dims, &num_of_dims, &format, &fixed_point_pos);

    status |= SetMMOutputMetaFormat (&metas[MATRIX_MULTIPLY_PARAM_TENSOR_OUT], out_dims, num_of_dims, format, fixed_point_pos);


    return status;
}


vx_kernel_description_t tensor_matrix_multiply_kernel  = {
     VX_KERNEL_TENSOR_MATRIX_MULTIPLY,
    "org.khronos.openvx.tensor_matrix_multiply",
    tensorMultiplyMatrixKernel,
    tensor_matrix_multiply_kernel_params, dimof(tensor_matrix_multiply_kernel_params),
    tensorMatrixMultiplyValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};



