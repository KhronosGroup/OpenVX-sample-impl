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

#include <c_model.h>
#include <stdlib.h>


static vx_status logical_operation(vx_enum operation, vx_scalar a, vx_scalar b, vx_scalar output);
static vx_status comparison_operation(vx_enum operation, vx_scalar a, vx_scalar b, vx_scalar output);
static vx_status arithmetic_operation(vx_enum operation, vx_scalar a, vx_scalar b, vx_scalar output);
static vx_status modulus_operation(vx_scalar a, vx_scalar b, vx_scalar output);

vx_status vxScalarOperation(vx_scalar scalar_operation, vx_scalar a, vx_scalar b, vx_scalar output)
{
    vx_status status = VX_SUCCESS;
    vx_enum operation = -1;
    vxCopyScalar(scalar_operation, &operation, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    switch( operation )
    {
        case VX_SCALAR_OP_AND:
        case VX_SCALAR_OP_OR:
        case VX_SCALAR_OP_XOR:
        case VX_SCALAR_OP_NAND:
        {
            status = logical_operation(operation,a, b, output);
            break;
        }
        case VX_SCALAR_OP_EQUAL:
        case VX_SCALAR_OP_NOTEQUAL:
        case VX_SCALAR_OP_LESS:
        case VX_SCALAR_OP_LESSEQ:
        case VX_SCALAR_OP_GREATER:
        case VX_SCALAR_OP_GREATEREQ:
        {
            status = comparison_operation(operation, a, b, output);
            break;
        }
        case VX_SCALAR_OP_ADD:
        case VX_SCALAR_OP_SUBTRACT:
        case VX_SCALAR_OP_MULTIPLY:
        case VX_SCALAR_OP_DIVIDE:
        case VX_SCALAR_OP_MIN:
        case VX_SCALAR_OP_MAX:
        {
            status = arithmetic_operation(operation, a, b, output);
            break;
        }
        case VX_SCALAR_OP_MODULUS:
        {
            status = modulus_operation(a, b, output);
            break;
        }
        default:
            status = VX_ERROR_NOT_IMPLEMENTED;
            break;
    }
    return status;
}

vx_status vxSelect(vx_scalar condition_param, vx_reference true_value, vx_reference false_value, vx_reference output)
{
    vx_status status = VX_SUCCESS;
    vx_bool condition = vx_false_e;
    status = vxCopyScalar(condition_param, &condition, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if(condition == vx_true_e)
    {
        status = vxCopy(true_value, output);
    }
    else if(condition == vx_false_e)
    {
        status = vxCopy(false_value, output);
    }
    else
    {
        status = VX_ERROR_INVALID_VALUE;
    }
    return status;
}

static vx_status logical_operation(vx_enum operation, vx_scalar a, vx_scalar b, vx_scalar output)
{
    vx_status status = VX_SUCCESS;

    vx_enum a_type = VX_TYPE_INVALID;
    vx_enum b_type = VX_TYPE_INVALID;
    vx_enum o_type = VX_TYPE_INVALID;
    vxQueryScalar(a, VX_SCALAR_TYPE, &a_type, sizeof(a_type));
    vxQueryScalar(b, VX_SCALAR_TYPE, &b_type, sizeof(b_type));
    vxQueryScalar(output, VX_SCALAR_TYPE, &o_type, sizeof(o_type));

    if((a_type != VX_TYPE_BOOL) ||
       (b_type != VX_TYPE_BOOL) ||
       (o_type != VX_TYPE_BOOL) )
    {
        status = VX_ERROR_NOT_SUPPORTED;
        return status;
    }

    vx_bool a_value = vx_false_e;
    vx_bool b_value = vx_false_e;
    vx_bool o_value = vx_false_e;
    vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyScalar(b, &b_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyScalar(output, &o_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if(operation == VX_SCALAR_OP_AND)
    {
        o_value = (a_value && b_value) ? vx_true_e : vx_false_e;
        status = vxWriteScalarValue(output, &o_value);
    }
    else if(operation == VX_SCALAR_OP_OR)
    {
        o_value = (a_value || b_value) ? vx_true_e : vx_false_e;
        status = vxWriteScalarValue(output, &o_value);
    }
    else if(operation == VX_SCALAR_OP_XOR)
    {
        o_value = ((a_value && !b_value) || (!a_value && b_value)) ? vx_true_e : vx_false_e;
        status = vxWriteScalarValue(output, &o_value);
    }
    else if(operation == VX_SCALAR_OP_NAND)
    {
        o_value = (!(a_value && b_value)) ? vx_true_e : vx_false_e;
        status = vxWriteScalarValue(output, &o_value);
    }
    else
    {
        status = VX_ERROR_NOT_SUPPORTED;
    }
    return status;
}

// create the following scalar read and convert functions:
//    vx_float32 read_and_convert_to_vx_float32(vx_scalar a);
//    vx_int32   read_and_convert_to_vx_int32(vx_scalar a);
//    vx_size    read_and_convert_to_vx_size(vx_scalar a);
//
#define CREATE_READ_AND_CONVERT_FUNCTION(OUTPUT_TYPE) \
static OUTPUT_TYPE read_and_convert_to_ ## OUTPUT_TYPE (vx_scalar a) \
{ \
    OUTPUT_TYPE value = 0; \
    vx_enum a_type = VX_TYPE_INVALID; \
    vxQueryScalar(a, VX_SCALAR_TYPE, &a_type, sizeof(a_type)); \
    switch (a_type) \
    { \
        case VX_TYPE_FLOAT32: \
        { \
            vx_float32 a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        case VX_TYPE_INT8: \
        { \
            vx_int8 a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        case VX_TYPE_UINT8: \
        { \
            vx_uint8 a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        case VX_TYPE_INT16: \
        { \
            vx_int16 a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        case VX_TYPE_UINT16: \
        { \
            vx_uint16 a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        case VX_TYPE_INT32: \
        { \
            vx_int32 a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        case VX_TYPE_UINT32: \
        { \
            vx_uint32 a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        case VX_TYPE_SIZE: \
        { \
            vx_size a_value; \
            vxCopyScalar(a, &a_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST); \
            value = (OUTPUT_TYPE)a_value; \
            break; \
        } \
        default: \
            break; \
    } \
    return value; \
}
CREATE_READ_AND_CONVERT_FUNCTION(vx_float32)
CREATE_READ_AND_CONVERT_FUNCTION(vx_int32)
CREATE_READ_AND_CONVERT_FUNCTION(vx_size)

// create the following convert and scalar write functions:
//    vx_status convert_and_write_vx_float32(vx_float32 value, vx_scalar a);
//    vx_status convert_and_write_vx_int32(vx_int32 value, vx_scalar a);
//    vx_status convert_and_write_vx_size(vx_size value, vx_scalar a);
//
#define CREATE_CONVERT_AND_WRITE_FUNCTION(INPUT_TYPE) \
static vx_status convert_and_write_ ## INPUT_TYPE (INPUT_TYPE value, vx_scalar a) \
{ \
    vx_status status = VX_SUCCESS; \
    vx_enum a_type = VX_TYPE_INVALID; \
    vxQueryScalar(a, VX_SCALAR_TYPE, &a_type, sizeof(a_type)); \
    switch (a_type) \
    { \
        case VX_TYPE_FLOAT32: \
        { \
            vx_float32 a_value = (vx_float32)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        case VX_TYPE_INT8: \
        { \
            vx_int8 a_value = (vx_int8)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        case VX_TYPE_UINT8: \
        { \
            vx_uint8 a_value = (vx_uint8)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        case VX_TYPE_INT16: \
        { \
            vx_int16 a_value = (vx_int16)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        case VX_TYPE_UINT16: \
        { \
            vx_uint16 a_value = (vx_uint16)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        case VX_TYPE_INT32: \
        { \
            vx_int32 a_value = (vx_int32)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        case VX_TYPE_UINT32: \
        { \
            vx_uint32 a_value = (vx_uint32)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        case VX_TYPE_SIZE: \
        { \
            vx_size a_value = (vx_size)value; \
            status = vxCopyScalar(a, &a_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); \
            break; \
        } \
        default: \
            status = VX_ERROR_NOT_SUPPORTED; \
            break; \
    } \
    return status; \
}
CREATE_CONVERT_AND_WRITE_FUNCTION(vx_float32)
CREATE_CONVERT_AND_WRITE_FUNCTION(vx_int32)
CREATE_CONVERT_AND_WRITE_FUNCTION(vx_size)

// create the following scalar comparison functions:
//    vx_bool compare_operation_vx_float32(vx_enum operation, vx_float32 a, vx_float32 b);
//    vx_bool compare_operation_vx_int32(vx_enum operation, vx_int32 a, vx_int32 b);
//    vx_bool compare_operation_vx_size(vx_enum operation, vx_size a, vx_size b);
//
#define CREATE_COMPARE_OPERATION(INPUT_TYPE) \
static vx_bool compare_operation_ ## INPUT_TYPE (vx_enum operation, INPUT_TYPE a_value, INPUT_TYPE b_value) \
{ \
    vx_bool o_value = vx_false_e; \
    switch(operation) \
    { \
        case VX_SCALAR_OP_EQUAL: \
            o_value = (a_value == b_value) ? vx_true_e : vx_false_e; \
            break; \
        case VX_SCALAR_OP_NOTEQUAL: \
            o_value = (a_value != b_value) ? vx_true_e : vx_false_e; \
            break; \
        case VX_SCALAR_OP_LESS: \
            o_value = (a_value < b_value) ? vx_true_e : vx_false_e; \
            break; \
        case VX_SCALAR_OP_LESSEQ: \
            o_value = (a_value <= b_value) ? vx_true_e : vx_false_e; \
            break; \
        case VX_SCALAR_OP_GREATER: \
            o_value = (a_value > b_value) ? vx_true_e : vx_false_e; \
            break; \
        case VX_SCALAR_OP_GREATEREQ: \
            o_value = (a_value >= b_value) ? vx_true_e : vx_false_e; \
            break; \
        default: \
            break; \
    } \
    return o_value; \
}
CREATE_COMPARE_OPERATION(vx_float32)
CREATE_COMPARE_OPERATION(vx_int32)
CREATE_COMPARE_OPERATION(vx_size)

// create the following scalar arighmetic functions:
//    vx_bool arithmetic_operation_vx_float32(vx_enum operation, vx_float32 a, vx_float32 b);
//    vx_bool arithmetic_operation_vx_int32(vx_enum operation, vx_int32 a, vx_int32 b);
//    vx_bool arithmetic_operation_vx_size(vx_enum operation, vx_size a, vx_size b);
//
#define CREATE_ARITHMETIC_OPERATION(INPUT_TYPE) \
static INPUT_TYPE arithmetic_operation_ ## INPUT_TYPE (vx_enum operation, INPUT_TYPE a_value, INPUT_TYPE b_value) \
{ \
    INPUT_TYPE o_value = 0; \
    switch(operation) \
    { \
        case VX_SCALAR_OP_ADD: \
            o_value = (a_value + b_value); \
            break; \
        case VX_SCALAR_OP_SUBTRACT: \
            o_value = (a_value - b_value); \
            break; \
        case VX_SCALAR_OP_MULTIPLY: \
            o_value = (a_value * b_value); \
            break; \
        case VX_SCALAR_OP_DIVIDE: \
            o_value = (b_value != 0) ? (a_value / b_value) : 0; \
            break; \
        case VX_SCALAR_OP_MIN: \
            if(a_value > b_value) \
                o_value = b_value; \
            else \
                o_value = a_value; \
            break; \
        case VX_SCALAR_OP_MAX: \
            if(a_value > b_value) \
               o_value = a_value; \
            else \
               o_value = b_value; \
            break; \
        default: \
            break; \
    } \
    return o_value; \
}
CREATE_ARITHMETIC_OPERATION(vx_float32)
CREATE_ARITHMETIC_OPERATION(vx_int32)
CREATE_ARITHMETIC_OPERATION(vx_size)

static vx_status comparison_operation(vx_enum operation, vx_scalar a, vx_scalar b, vx_scalar output)
{
    vx_status status = VX_SUCCESS;
    vx_enum a_type = VX_TYPE_INVALID;
    vx_enum b_type = VX_TYPE_INVALID;
    vx_enum o_type = VX_TYPE_INVALID;
    vxQueryScalar(a, VX_SCALAR_TYPE, &a_type, sizeof(a_type));
    vxQueryScalar(b, VX_SCALAR_TYPE, &b_type, sizeof(b_type));
    vxQueryScalar(output, VX_SCALAR_TYPE, &o_type, sizeof(o_type));

    if(o_type != VX_TYPE_BOOL)
    {
        status = VX_ERROR_NOT_SUPPORTED;
        return status;
    }

    if(a_type == VX_TYPE_FLOAT32 || b_type == VX_TYPE_FLOAT32)
    {
        vx_float32 a_value = read_and_convert_to_vx_float32(a);
        vx_float32 b_value = read_and_convert_to_vx_float32(b);
        vx_bool o_value = compare_operation_vx_float32(operation, a_value, b_value);
        status = vxCopyScalar(output, &o_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    }
    else if(a_type == VX_TYPE_INT8 || a_type == VX_TYPE_INT16 || a_type == VX_TYPE_INT32 ||
            b_type == VX_TYPE_INT8 || b_type == VX_TYPE_INT16 || b_type == VX_TYPE_INT32)
    {
        vx_int32 a_value = read_and_convert_to_vx_int32(a);
        vx_int32 b_value = read_and_convert_to_vx_int32(b);
        vx_bool o_value = compare_operation_vx_int32(operation, a_value, b_value);
        status = vxCopyScalar(output, &o_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    }
    else
    {
        vx_size a_value = read_and_convert_to_vx_size(a);
        vx_size b_value = read_and_convert_to_vx_size(b);
        vx_bool o_value = compare_operation_vx_size(operation, a_value, b_value);
        status = vxCopyScalar(output, &o_value, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    }

    return status;
}

static vx_status arithmetic_operation(vx_enum operation, vx_scalar a, vx_scalar b, vx_scalar output)
{
    vx_status status = VX_SUCCESS;
    vx_enum a_type = VX_TYPE_INVALID;
    vx_enum b_type = VX_TYPE_INVALID;
    vx_enum o_type = VX_TYPE_INVALID;
    vxQueryScalar(a, VX_SCALAR_TYPE, &a_type, sizeof(a_type));
    vxQueryScalar(b, VX_SCALAR_TYPE, &b_type, sizeof(b_type));
    vxQueryScalar(output, VX_SCALAR_TYPE, &o_type, sizeof(o_type));

    // TODO: need special handling for mixing VX_SIZE and integer for 64-bit hardware
    if(a_type == VX_TYPE_FLOAT32 || b_type == VX_TYPE_FLOAT32)
    {
        vx_float32 a_value = read_and_convert_to_vx_float32(a);
        vx_float32 b_value = read_and_convert_to_vx_float32(b);
        vx_float32 o_value = arithmetic_operation_vx_float32(operation, a_value, b_value);
        status = convert_and_write_vx_float32(o_value, output);
    }
    else if(a_type == VX_TYPE_INT8 || a_type == VX_TYPE_INT16 || a_type == VX_TYPE_INT32 ||
            b_type == VX_TYPE_INT8 || b_type == VX_TYPE_INT16 || b_type == VX_TYPE_INT32)
    {
        vx_int32 a_value = read_and_convert_to_vx_int32(a);
        vx_int32 b_value = read_and_convert_to_vx_int32(b);
        vx_int32 o_value = arithmetic_operation_vx_int32(operation, a_value, b_value);
        status = convert_and_write_vx_int32(o_value, output);
    }
    else
    {
        vx_size a_value = read_and_convert_to_vx_size(a);
        vx_size b_value = read_and_convert_to_vx_size(b);
        vx_size o_value = arithmetic_operation_vx_size(operation, a_value, b_value);
        status = convert_and_write_vx_size(o_value, output);
    }

    return status;
}

static vx_status modulus_operation(vx_scalar a, vx_scalar b, vx_scalar output)
{
    vx_status status = VX_SUCCESS;
    vx_enum a_type = VX_TYPE_INVALID;
    vx_enum b_type = VX_TYPE_INVALID;
    vx_enum o_type = VX_TYPE_INVALID;
    vxQueryScalar(a, VX_SCALAR_TYPE, &a_type, sizeof(a_type));
    vxQueryScalar(b, VX_SCALAR_TYPE, &b_type, sizeof(b_type));
    vxQueryScalar(output, VX_SCALAR_TYPE, &o_type, sizeof(o_type));

    if(a_type == VX_TYPE_FLOAT32 || b_type == VX_TYPE_FLOAT32)
    {
        status = VX_ERROR_NOT_SUPPORTED;
    }
    else if(a_type == VX_TYPE_INT8 || a_type == VX_TYPE_INT16 || a_type == VX_TYPE_INT32 ||
            b_type == VX_TYPE_INT8 || b_type == VX_TYPE_INT16 || b_type == VX_TYPE_INT32)
    {
        vx_int32 a_value = read_and_convert_to_vx_int32(a);
        vx_int32 b_value = read_and_convert_to_vx_int32(b);
        vx_int32 o_value = (b_value != 0) ? (a_value % b_value) : 0;
        status = convert_and_write_vx_int32(o_value, output);
    }
    else
    {
        vx_size a_value = read_and_convert_to_vx_size(a);
        vx_size b_value = read_and_convert_to_vx_size(b);
        vx_size o_value = (b_value != 0) ? (a_value % b_value) : 0;
        status = convert_and_write_vx_size(o_value, output);
    }

    return status;
}
