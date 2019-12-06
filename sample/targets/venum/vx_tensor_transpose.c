/*

 * Copyright (c) 2012-2017 The Khronos Group Inc.
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




/*==============================================================================
==============================================================================
KERNELS AND INPUT/OUTPUT VALIDATORS
==============================================================================
=============================================================================*/
typedef enum _transpose_params_e {
    TRANSPOSE_PARAM_TENSOR_IN = 0,
    TRANSPOSE_PARAM_DIM1,
    TRANSPOSE_PARAM_DIM2,
    TRANSPOSE_PARAM_TENSOR_OUT,
    TRANSPOSE_PARAMS_NUMBER
} transpose_params_e;


static vx_status VX_CALLBACK tensorTansposeKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == TRANSPOSE_PARAMS_NUMBER)
    {
        vx_tensor in = (vx_tensor)parameters[TRANSPOSE_PARAM_TENSOR_IN];
        vx_scalar dim1 = (vx_scalar)parameters[TRANSPOSE_PARAM_DIM1];
        vx_scalar dim2 = (vx_scalar)parameters[TRANSPOSE_PARAM_DIM2];
        vx_tensor output = (vx_tensor)parameters[TRANSPOSE_PARAM_TENSOR_OUT];
        status = TransposeTensorKernelImpl(in, dim1, dim2, output, ownSizeOfType(output->data_type));
    }

    return status;
}



static vx_status SetTransposeOutputMetaFormat (vx_tensor in1, vx_tensor out, vx_size out_dims[], vx_meta_format * meta)
{
    vx_status status = VX_SUCCESS;
    vx_size out_num_dims;
    vx_enum format1, format_out;
    vx_uint8 fixed_point_pos1 = 0, fixed_point_pos_out = 0;

    vxQueryTensor(in1, VX_TENSOR_DATA_TYPE, &format1, sizeof(format1));
    vxQueryTensor(in1, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos1, sizeof(fixed_point_pos1));


    vxQueryTensor(out, VX_TENSOR_DATA_TYPE, &format_out, sizeof(format_out));
    vxQueryTensor(out, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos_out, sizeof(fixed_point_pos_out));

    if ((format_out != VX_TYPE_INVALID && format_out != format1) ||
            (fixed_point_pos_out != 0 && fixed_point_pos_out != fixed_point_pos1) ||
            (format1 != VX_TYPE_INT16 && format1 != VX_TYPE_UINT8 && format1 != VX_TYPE_INT8) ||
            (format1 == VX_TYPE_INT16 && fixed_point_pos1 != 0 && fixed_point_pos1 != Q78_FIXED_POINT_POSITION))
    {
        status = VX_ERROR_INVALID_FORMAT;
    } else {
        status |= vxQueryTensor(in1, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
        if (status == VX_SUCCESS)
        {
            status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DATA_TYPE, &format1, sizeof(format1));
            status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos1, sizeof(fixed_point_pos1));
            status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_DIMS, out_dims, sizeof(*out_dims) * out_num_dims);
            status |= vxSetMetaFormatAttribute(*meta, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));
        }
    }


    return status;
}

static vx_status validateDimensions(vx_tensor in, vx_tensor out, vx_scalar sc_dim1, vx_scalar sc_dim2, vx_size out_dims[])
{
    vx_status status = VX_SUCCESS;
    vx_size in1_dim[VX_MAX_TENSOR_DIMENSIONS];
    vx_size in1_num_dims, out_num_dims;
    vx_size dim1, dim2;
    status = vxCopyScalar(sc_dim1, &dim1, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(sc_dim2, &dim2, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    status |= vxQueryTensor(in, VX_TENSOR_NUMBER_OF_DIMS, &in1_num_dims, sizeof(in1_num_dims));
    status |= vxQueryTensor(out, VX_TENSOR_NUMBER_OF_DIMS, &out_num_dims, sizeof(out_num_dims));

    //TODO: this is exactly the kind of issue I mentioned earlier: here we possibly query more dims than the tensor actually has. should it be filled with 0s or must we query only sizeof(in1_dim[0]) * in1_num_dims bytes?
    status |= vxQueryTensor(in, VX_TENSOR_DIMS, in1_dim, sizeof (in1_dim));
    status |= vxQueryTensor(out, VX_TENSOR_DIMS, out_dims, sizeof (*out_dims) * out_num_dims);

    if (out_num_dims != 0)
    {
        if (in1_num_dims != out_num_dims)
        {
            status = VX_ERROR_INVALID_DIMENSION;
        }
        else if (dim1 >= out_num_dims || dim2 >= out_num_dims)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        else
        {
            for (int i = 0; i < out_num_dims; i++)
            {
                if (out_dims[i] == 0)
                {
                    if (i == dim1)
                        out_dims[i] = in1_dim[dim2];
                    else if (i == dim2)
                        out_dims[i] = in1_dim[dim1];
                    else
                        out_dims[i] = in1_dim[i];

                }
                else if ((i != dim1 && i != dim2 && in1_dim[i] != out_dims[i]) ||
                        (i == dim1 && in1_dim[i] != out_dims[dim2]) ||
                        (i == dim2 && in1_dim[i] != out_dims[dim1]))
                {
                    status = VX_ERROR_INVALID_DIMENSION;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < in1_num_dims; i++)
        {
            if (i == dim1)
                out_dims[i] = in1_dim[dim2];
            else if (i == dim2)
                out_dims[i] = in1_dim[dim1];
            else
                out_dims[i] = in1_dim[i];
        }
    }
    return status;
}

/*! \brief Transpose parameters validator.
 * \param [in] node The handle to the node.
 * \param [in] parameters The array of parameters to be validated.
 * \param [in] num Number of parameters to be validated.
 * \param [out] metas The array of metadata used to check output parameters only.
 * \return A \ref vx_status_e enumeration.
 * \ingroup group_example_kernel
 */
static vx_status VX_CALLBACK tensorTransposeValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    if (num != TRANSPOSE_PARAMS_NUMBER)
        return VX_ERROR_INVALID_PARAMETERS;
    // Check input tensors
    vx_tensor in = (vx_tensor)parameters[TRANSPOSE_PARAM_TENSOR_IN];
    vx_tensor out = (vx_tensor)parameters[TRANSPOSE_PARAM_TENSOR_OUT];
    vx_status status = VX_SUCCESS;
    vx_size out_dims[VX_MAX_TENSOR_DIMENSIONS];
    status = validateDimensions(in,out, (vx_scalar)parameters[TRANSPOSE_PARAM_DIM1], (vx_scalar)parameters[TRANSPOSE_PARAM_DIM2], out_dims);
    status |= SetTransposeOutputMetaFormat (in, out, out_dims, &metas[TRANSPOSE_PARAM_TENSOR_OUT]);


    return status;
}

static vx_param_description_t tensor_transpose_kernel_params[] = {
    { VX_INPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_TENSOR, VX_PARAMETER_STATE_REQUIRED },
};


vx_kernel_description_t tensor_transpose_kernel  = {
    VX_KERNEL_TENSOR_TRANSPOSE,
    "org.khronos.openvx.tensor_transpose",
    tensorTansposeKernel,
    tensor_transpose_kernel_params, dimof(tensor_transpose_kernel_params),
    tensorTransposeValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};

