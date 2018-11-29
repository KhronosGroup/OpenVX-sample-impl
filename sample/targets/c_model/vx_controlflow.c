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

#include <vx_internal.h>
#include <c_model.h>

static vx_param_description_t scalar_operation_kernel_params[] = {
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
};

static vx_status VX_CALLBACK vxScalarOperationParamsValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL == node || NULL == parameters || num != dimof(scalar_operation_kernel_params) || NULL == metas)
    {
        return status;
    }

    vx_enum stype0 = 0;
    vx_enum operation = 0;
    vx_parameter param0 = vxGetParameterByIndex(node, 0);
    if (vxGetStatus((vx_reference)param0) == VX_SUCCESS)
    {
        vx_scalar scalar = 0;
        vxQueryParameter(param0, VX_PARAMETER_REF, &scalar, sizeof(scalar));
        if (scalar)
        {
            vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype0, sizeof(stype0));
            if (stype0 == VX_TYPE_ENUM)
            {
                vxCopyScalar(scalar, &operation, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                if ((operation == VX_SCALAR_OP_AND) ||
                    (operation == VX_SCALAR_OP_OR)  ||
                    (operation == VX_SCALAR_OP_XOR) ||
                    (operation == VX_SCALAR_OP_NAND)  ||
                    (operation == VX_SCALAR_OP_EQUAL) ||
                    (operation == VX_SCALAR_OP_NOTEQUAL)  ||
                    (operation == VX_SCALAR_OP_LESS) ||
                    (operation == VX_SCALAR_OP_LESSEQ)  ||
                    (operation == VX_SCALAR_OP_GREATER) ||
                    (operation == VX_SCALAR_OP_GREATEREQ)  ||
                    (operation == VX_SCALAR_OP_ADD) ||
                    (operation == VX_SCALAR_OP_SUBTRACT) ||
                    (operation == VX_SCALAR_OP_MULTIPLY) ||
                    (operation == VX_SCALAR_OP_DIVIDE) ||
                    (operation == VX_SCALAR_OP_MODULUS) ||
                    (operation == VX_SCALAR_OP_MIN) ||
                    (operation == VX_SCALAR_OP_MAX))
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
            vxReleaseScalar(&scalar);
        }
        vxReleaseParameter(&param0);
    }

    if(status != VX_SUCCESS)
    {
        return status;
    }

    vx_enum stype1 = 0;
    vx_parameter param1 = vxGetParameterByIndex(node, 1);
    if (vxGetStatus((vx_reference)param1) == VX_SUCCESS)
    {
        vx_scalar scalar = 0;
        vxQueryParameter(param1, VX_PARAMETER_REF, &scalar, sizeof(scalar));
        if (scalar)
        {
            vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype1, sizeof(stype1));
            vxReleaseScalar(&scalar);
        }
        vxReleaseParameter(&param1);
    }

    vx_enum stype2 = 0;
    vx_parameter param2 = vxGetParameterByIndex(node, 2);
    if (vxGetStatus((vx_reference)param2) == VX_SUCCESS)
    {
        vx_scalar scalar = 0;
        vxQueryParameter(param2, VX_PARAMETER_REF, &scalar, sizeof(scalar));
        if (scalar)
        {
            vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype2, sizeof(stype2));
            vxReleaseScalar(&scalar);
        }
        vxReleaseParameter(&param2);
    }

    vx_enum stype3 = 0;
    vx_parameter param3 = vxGetParameterByIndex(node, 3);
    if (vxGetStatus((vx_reference)param3) == VX_SUCCESS)
    {
        vx_scalar scalar = 0;
        vxQueryParameter(param3, VX_PARAMETER_REF, &scalar, sizeof(scalar));
        if (scalar)
        {
            vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype3, sizeof(stype3));
            vxReleaseScalar(&scalar);
        }
        vxReleaseParameter(&param3);
    }

    if(stype1 == 0 || stype2 == 0 || stype3 == 0)
    {
        status = VX_ERROR_INVALID_VALUE;
        return status;
    }

    switch(operation)
    {
    case VX_SCALAR_OP_AND:
    case VX_SCALAR_OP_OR:
    case VX_SCALAR_OP_XOR:
    case VX_SCALAR_OP_NAND:
    {
        if((stype1 == VX_TYPE_BOOL) && (stype2 == VX_TYPE_BOOL) && (stype3 == VX_TYPE_BOOL))
        {
            status = VX_SUCCESS;
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        break;
    }
    case VX_SCALAR_OP_EQUAL:
    case VX_SCALAR_OP_NOTEQUAL:
    case VX_SCALAR_OP_LESS:
    case VX_SCALAR_OP_LESSEQ:
    case VX_SCALAR_OP_GREATER:
    case VX_SCALAR_OP_GREATEREQ:
    {
        if((stype1 == VX_TYPE_INT8   ||
            stype1 == VX_TYPE_UINT8  ||
            stype1 == VX_TYPE_INT16  ||
            stype1 == VX_TYPE_UINT16 ||
            stype1 == VX_TYPE_INT32  ||
            stype1 == VX_TYPE_UINT32 ||
            stype1 == VX_TYPE_SIZE   ||
            stype1 == VX_TYPE_FLOAT32) &&
           (stype2 == VX_TYPE_INT8   ||
            stype2 == VX_TYPE_UINT8  ||
            stype2 == VX_TYPE_INT16  ||
            stype2 == VX_TYPE_UINT16 ||
            stype2 == VX_TYPE_INT32  ||
            stype2 == VX_TYPE_UINT32 ||
            stype2 == VX_TYPE_SIZE   ||
            stype2 == VX_TYPE_FLOAT32) &&
           (stype3 == VX_TYPE_BOOL   ))
        {
            status = VX_SUCCESS;
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        break;
    }
    case VX_SCALAR_OP_MODULUS:
    {
        if((stype1 == VX_TYPE_INT8   ||
            stype1 == VX_TYPE_UINT8  ||
            stype1 == VX_TYPE_INT16  ||
            stype1 == VX_TYPE_UINT16 ||
            stype1 == VX_TYPE_INT32  ||
            stype1 == VX_TYPE_UINT32 ||
            stype1 == VX_TYPE_SIZE  ) &&
           (stype2 == VX_TYPE_INT8   ||
            stype2 == VX_TYPE_UINT8  ||
            stype2 == VX_TYPE_INT16  ||
            stype2 == VX_TYPE_UINT16 ||
            stype2 == VX_TYPE_INT32  ||
            stype2 == VX_TYPE_UINT32 ||
            stype2 == VX_TYPE_SIZE  ) &&
           (stype3 == VX_TYPE_INT8   ||
            stype3 == VX_TYPE_UINT8  ||
            stype3 == VX_TYPE_INT16  ||
            stype3 == VX_TYPE_UINT16 ||
            stype3 == VX_TYPE_INT32  ||
            stype3 == VX_TYPE_UINT32 ||
            stype3 == VX_TYPE_SIZE   ||
            stype3 == VX_TYPE_FLOAT32))
        {
            status = VX_SUCCESS;
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        break;
    }
    case VX_SCALAR_OP_ADD:
    case VX_SCALAR_OP_SUBTRACT:
    case VX_SCALAR_OP_MULTIPLY:
    case VX_SCALAR_OP_DIVIDE:
    case VX_SCALAR_OP_MIN:
    case VX_SCALAR_OP_MAX:
    {
        if((stype1 == VX_TYPE_INT8   ||
            stype1 == VX_TYPE_UINT8  ||
            stype1 == VX_TYPE_INT16  ||
            stype1 == VX_TYPE_UINT16 ||
            stype1 == VX_TYPE_INT32  ||
            stype1 == VX_TYPE_UINT32 ||
            stype1 == VX_TYPE_SIZE   ||
            stype1 == VX_TYPE_FLOAT32) &&
           (stype2 == VX_TYPE_INT8   ||
            stype2 == VX_TYPE_UINT8  ||
            stype2 == VX_TYPE_INT16  ||
            stype2 == VX_TYPE_UINT16 ||
            stype2 == VX_TYPE_INT32  ||
            stype2 == VX_TYPE_UINT32 ||
            stype2 == VX_TYPE_SIZE   ||
            stype2 == VX_TYPE_FLOAT32) &&
           (stype3 == VX_TYPE_INT8   ||
            stype3 == VX_TYPE_UINT8  ||
            stype3 == VX_TYPE_INT16  ||
            stype3 == VX_TYPE_UINT16 ||
            stype3 == VX_TYPE_INT32  ||
            stype3 == VX_TYPE_UINT32 ||
            stype3 == VX_TYPE_SIZE   ||
            stype3 == VX_TYPE_FLOAT32))
        {
            status = VX_SUCCESS;
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        break;
    }
    default:
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        break;
    }
    }

    if(status != VX_SUCCESS)
    {
        return status;
    }

    vx_meta_format_t *ptr = metas[3];
    switch (stype3)
    {
    case VX_TYPE_CHAR:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_CHAR;
        break;
    case VX_TYPE_INT8:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_INT8;
        break;
    case VX_TYPE_UINT8:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_UINT8;
        break;
    case VX_TYPE_INT16:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_INT16;
        break;
    case VX_TYPE_UINT16:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_UINT16;
        break;
    case VX_TYPE_INT32:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_INT32;
        break;
    case VX_TYPE_UINT32:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_UINT32;
        break;
    case VX_TYPE_INT64:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_INT64;
        break;
    case VX_TYPE_UINT64:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_UINT64;
        break;
    case VX_TYPE_FLOAT32:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_FLOAT32;
        break;
    case VX_TYPE_FLOAT64:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_FLOAT64;
        break;
    case VX_TYPE_DF_IMAGE:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_IMAGE;
        break;
    case VX_TYPE_ENUM:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_ENUM;
        break;
    case VX_TYPE_SIZE:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_SIZE;
        break;
    case VX_TYPE_BOOL:
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_BOOL;
        break;
    }

    return status;
}

static vx_status VX_CALLBACK vxScalarOperationKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == dimof(scalar_operation_kernel_params))
    {
        vx_scalar scalar_operation = (vx_scalar)parameters[0];
        vx_scalar a = (vx_scalar)parameters[1];
        vx_scalar b = (vx_scalar)parameters[2];
        vx_scalar output = (vx_scalar)parameters[3];
        return vxScalarOperation(scalar_operation, a, b, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t scalar_operation_kernel = {
    VX_KERNEL_SCALAR_OPERATION,
    "org.khronos.openvx.scalar_operation",
    vxScalarOperationKernel,
    scalar_operation_kernel_params, dimof(scalar_operation_kernel_params),
    vxScalarOperationParamsValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};


static vx_param_description_t select_kernel_params[] = {
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_REFERENCE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_REFERENCE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_REFERENCE, VX_PARAMETER_STATE_REQUIRED},
};

static vx_status VX_CALLBACK vxSelectParamsValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL == node || NULL == parameters || num != dimof(select_kernel_params) || NULL == metas)
    {
        return status;
    }

    vx_parameter param0 = vxGetParameterByIndex(node, 0);
    if (vxGetStatus((vx_reference)param0) == VX_SUCCESS)
    {
        vx_scalar condition_scalar = 0;
        status = vxQueryParameter(param0, VX_PARAMETER_REF, &condition_scalar, sizeof(condition_scalar));
        if ((status == VX_SUCCESS) && (condition_scalar))
        {
            vx_enum type0 = VX_TYPE_INVALID;
            vxQueryScalar(condition_scalar, VX_SCALAR_TYPE, &type0, sizeof(type0));
            if (type0 == VX_TYPE_BOOL)
            {
                vx_bool condition_ = vx_false_e;
                status = vxCopyScalar(condition_scalar, &condition_, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                if ((status == VX_SUCCESS) && ((condition_ == vx_false_e) ||
                                               (condition_ == vx_true_e)))
                {
                    status = VX_SUCCESS;
                }
            }
            else
            {
                status = VX_ERROR_INVALID_TYPE;
            }
            vxReleaseScalar(&condition_scalar);
        }
        vxReleaseParameter(&param0);
    }

    if(status != VX_SUCCESS)
    {
        return status;
    }

    vx_enum type1 = VX_TYPE_INVALID;
    vx_parameter param1 = vxGetParameterByIndex(node, 1);
    if (param1)
    {
        vx_reference ref1 = 0;
        vxQueryParameter(param1, VX_PARAMETER_REF, &ref1, sizeof(ref1));
        if (ref1)
        {
            vxQueryReference(ref1, VX_REFERENCE_TYPE, &type1, sizeof(type1));
            if((type1 == VX_TYPE_IMAGE)||
               (type1 == VX_TYPE_ARRAY)||
               (type1 == VX_TYPE_PYRAMID)||
               (type1 == VX_TYPE_SCALAR) ||
               (type1 == VX_TYPE_MATRIX) ||
               (type1 == VX_TYPE_CONVOLUTION) ||
               (type1 == VX_TYPE_DISTRIBUTION) ||
               (type1 == VX_TYPE_REMAP) ||
               (type1 == VX_TYPE_LUT) ||
               (type1 == VX_TYPE_THRESHOLD) ||
               (type1 == VX_TYPE_TENSOR)||
               (type1 == VX_TYPE_OBJECT_ARRAY))
            {
                 status = VX_SUCCESS;
            }
            else
            {
                 status = VX_ERROR_INVALID_TYPE;
            }
            vxReleaseReference(&ref1);
        }
        vxReleaseParameter(&param1);
    }

    if(status != VX_SUCCESS)
    {
        return status;
    }

    vx_enum type2 = VX_TYPE_INVALID;
    vx_parameter param2 = vxGetParameterByIndex(node, 2);
    if (param2)
    {
        vx_reference ref2 = 0;
        vxQueryParameter(param2, VX_PARAMETER_REF, &ref2, sizeof(ref2));
        if (ref2)
        {
            vxQueryReference(ref2, VX_REFERENCE_TYPE, &type2, sizeof(type2));
            if((type2 == VX_TYPE_IMAGE)||
               (type2 == VX_TYPE_ARRAY)||
               (type2 == VX_TYPE_PYRAMID)||
               (type2 == VX_TYPE_SCALAR) ||
               (type2 == VX_TYPE_MATRIX) ||
               (type2 == VX_TYPE_CONVOLUTION) ||
               (type2 == VX_TYPE_DISTRIBUTION) ||
               (type2 == VX_TYPE_REMAP) ||
               (type2 == VX_TYPE_LUT) ||
               (type2 == VX_TYPE_THRESHOLD) ||
               (type2 == VX_TYPE_TENSOR)||
               (type2 == VX_TYPE_OBJECT_ARRAY))
            {
                 status = VX_SUCCESS;
            }
            else
            {
                 status = VX_ERROR_INVALID_TYPE;
            }
            vxReleaseReference(&ref2);
        }
        vxReleaseParameter(&param2);
    }

    if(status != VX_SUCCESS)
    {
        return status;
    }

    vx_enum type3 = VX_TYPE_INVALID;
    vx_parameter param3 = vxGetParameterByIndex(node, 3);
    if (param3)
    {
        vx_reference ref3 = 0;
        vxQueryParameter(param3, VX_PARAMETER_REF, &ref3, sizeof(ref3));
        if (ref3)
        {
            vxQueryReference(ref3, VX_REFERENCE_TYPE, &type3, sizeof(type3));
            if((type3 == VX_TYPE_IMAGE)||
               (type3 == VX_TYPE_ARRAY)||
               (type3 == VX_TYPE_PYRAMID)||
               (type3 == VX_TYPE_SCALAR) ||
               (type3 == VX_TYPE_MATRIX) ||
               (type3 == VX_TYPE_CONVOLUTION) ||
               (type3 == VX_TYPE_DISTRIBUTION) ||
               (type3 == VX_TYPE_REMAP) ||
               (type3 == VX_TYPE_LUT) ||
               (type3 == VX_TYPE_THRESHOLD) ||
               (type3 == VX_TYPE_TENSOR)||
               (type3 == VX_TYPE_OBJECT_ARRAY))
            {
                 status = VX_SUCCESS;
            }
            else
            {
                 status = VX_ERROR_INVALID_TYPE;
            }
            vxReleaseReference(&ref3);
        }
        vxReleaseParameter(&param3);
    }

    if((status == VX_SUCCESS) && (type1 != VX_TYPE_INVALID) && (type1 == type2) && (type2 == type3))
    {
        status = VX_SUCCESS;
    }
    else
    {
        status = VX_ERROR_INVALID_TYPE;
    }

    return status;
}

static vx_status VX_CALLBACK vxSelectKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == 4)
    {
        vx_scalar condition = (vx_scalar)parameters[0];
        vx_reference true_value = (vx_reference)parameters[1];
        vx_reference false_value = (vx_reference)parameters[2];
        vx_reference output = (vx_reference)parameters[3];
        return vxSelect(condition, true_value, false_value, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t select_kernel = {
    VX_KERNEL_SELECT,
    "org.khronos.openvx.select",
    vxSelectKernel,
    select_kernel_params, dimof(select_kernel_params),
    vxSelectParamsValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};
