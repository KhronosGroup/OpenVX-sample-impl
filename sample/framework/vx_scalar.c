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

#include "vx_internal.h"
#include "vx_scalar.h"

void vxPrintScalarValue(vx_scalar scalar)
{
    switch (scalar->data_type)
    {
        case VX_TYPE_CHAR:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %c\n", scalar, scalar->data.chr);
            break;
        case VX_TYPE_INT8:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %d\n", scalar, scalar->data.s08);
            break;
        case VX_TYPE_UINT8:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %u\n", scalar, scalar->data.u08);
            break;
        case VX_TYPE_INT16:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %hd\n", scalar, scalar->data.s16);
            break;
        case VX_TYPE_UINT16:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %hu\n", scalar, scalar->data.u16);
            break;
        case VX_TYPE_INT32:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %d\n", scalar, scalar->data.s32);
            break;
        case VX_TYPE_UINT32:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %u\n", scalar, scalar->data.u32);
            break;
        case VX_TYPE_INT64:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %ld\n", scalar, scalar->data.s64);
            break;
        case VX_TYPE_UINT64:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %lu\n", scalar, scalar->data.u64);
            break;
#if OVX_SUPPORT_HALF_FLOAT
        case VX_TYPE_FLOAT16:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %f\n", scalar, scalar->data.f16);
            break;
#endif
        case VX_TYPE_FLOAT32:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %lf\n", scalar, scalar->data.f32);
            break;
        case VX_TYPE_FLOAT64:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %lf\n", scalar, scalar->data.f64);
            break;
        case VX_TYPE_DF_IMAGE:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %08x\n", scalar, scalar->data.fcc);
            break;
        case VX_TYPE_ENUM:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %d\n", scalar, scalar->data.enm);
            break;
        case VX_TYPE_SIZE:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %zu\n", scalar, scalar->data.size);
            break;
        case VX_TYPE_BOOL:
            VX_PRINT(VX_ZONE_SCALAR, "scalar "VX_FMT_REF" = %s\n", scalar, (scalar->data.boolean == vx_true_e?"TRUE":"FALSE"));
            break;
        default:
            VX_PRINT(VX_ZONE_ERROR, "Case %08x is not covered!\n", scalar->data_type);
            DEBUG_BREAK();
            break;
    }

    return;
} /* vxPrintScalarValue() */

VX_API_ENTRY vx_scalar VX_API_CALL vxCreateScalar(vx_context context, vx_enum data_type, const void* ptr)
{
    vx_scalar scalar = NULL;

    if (ownIsValidContext(context) == vx_false_e)
        return NULL;

    if (!VX_TYPE_IS_SCALAR(data_type))
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid type to scalar\n");
        vxAddLogEntry(&context->base, VX_ERROR_INVALID_TYPE, "Invalid type to scalar\n");
        scalar = (vx_scalar)ownGetErrorObject(context, VX_ERROR_INVALID_TYPE);
    }
    else
    {
        scalar = (vx_scalar)ownCreateReference(context, VX_TYPE_SCALAR, VX_EXTERNAL, &context->base);
        if (vxGetStatus((vx_reference)scalar) == VX_SUCCESS && scalar->base.type == VX_TYPE_SCALAR)
        {
            scalar->data_type = data_type;
            vxCopyScalar(scalar, (void*)ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
        }
    }

    return (vx_scalar)scalar;
} /* vxCreateScalar() */

VX_API_ENTRY vx_status VX_API_CALL vxReleaseScalar(vx_scalar *s)
{
    return ownReleaseReferenceInt((vx_reference *)s, VX_TYPE_SCALAR, VX_EXTERNAL, NULL);
} /* vxReleaseScalar() */

VX_API_ENTRY vx_status VX_API_CALL vxQueryScalar(vx_scalar scalar, vx_enum attribute, void* ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    vx_scalar_t* pscalar = (vx_scalar_t*)scalar;

    if (ownIsValidSpecificReference(&pscalar->base,VX_TYPE_SCALAR) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    switch (attribute)
    {
        case VX_SCALAR_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
            {
                *(vx_enum*)ptr = pscalar->data_type;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;

        default:
            status = VX_ERROR_NOT_SUPPORTED;
            break;
    }

    return status;
} /* vxQueryScalar() */

static vx_status own_scalar_to_host_mem(vx_scalar scalar, void* user_ptr)
{
    vx_status status = VX_SUCCESS;

    if (vx_false_e == ownSemWait(&scalar->base.lock))
        return VX_ERROR_NO_RESOURCES;

    vxPrintScalarValue(scalar);

    switch (scalar->data_type)
    {
    case VX_TYPE_CHAR:     *(vx_char*)user_ptr = scalar->data.chr; break;
    case VX_TYPE_INT8:     *(vx_int8*)user_ptr = scalar->data.s08; break;
    case VX_TYPE_UINT8:    *(vx_uint8*)user_ptr = scalar->data.u08; break;
    case VX_TYPE_INT16:    *(vx_int16*)user_ptr = scalar->data.s16; break;
    case VX_TYPE_UINT16:   *(vx_uint16*)user_ptr = scalar->data.u16; break;
    case VX_TYPE_INT32:    *(vx_int32*)user_ptr = scalar->data.s32; break;
    case VX_TYPE_UINT32:   *(vx_uint32*)user_ptr = scalar->data.u32; break;
    case VX_TYPE_INT64:    *(vx_int64*)user_ptr = scalar->data.s64; break;
    case VX_TYPE_UINT64:   *(vx_uint64*)user_ptr = scalar->data.u64; break;
#if OVX_SUPPORT_HALF_FLOAT
    case VX_TYPE_FLOAT16:  *(vx_float16*)ptr = scalar->data.f16; break;
#endif
    case VX_TYPE_FLOAT32:  *(vx_float32*)user_ptr = scalar->data.f32; break;
    case VX_TYPE_FLOAT64:  *(vx_float64*)user_ptr = scalar->data.f64; break;
    case VX_TYPE_DF_IMAGE: *(vx_df_image*)user_ptr = scalar->data.fcc; break;
    case VX_TYPE_ENUM:     *(vx_enum*)user_ptr = scalar->data.enm; break;
    case VX_TYPE_SIZE:     *(vx_size*)user_ptr = scalar->data.size; break;
    case VX_TYPE_BOOL:     *(vx_bool*)user_ptr = scalar->data.boolean; break;

    default:
        VX_PRINT(VX_ZONE_ERROR, "some case is not covered in %s\n", __FUNCTION__);
        status = VX_ERROR_NOT_SUPPORTED;
        break;
    }

    if (vx_false_e == ownSemPost(&scalar->base.lock))
        return VX_ERROR_NO_RESOURCES;

    ownReadFromReference(&scalar->base);

    return status;
} /* own_scalar_to_host_mem() */

static vx_status own_host_mem_to_scalar(vx_scalar scalar, void* user_ptr)
{
    vx_status status = VX_SUCCESS;

    if (vx_false_e == ownSemWait(&scalar->base.lock))
        return VX_ERROR_NO_RESOURCES;

    switch (scalar->data_type)
    {
    case VX_TYPE_CHAR:     scalar->data.chr = *(vx_char*)user_ptr; break;
    case VX_TYPE_INT8:     scalar->data.s08 = *(vx_int8*)user_ptr; break;
    case VX_TYPE_UINT8:    scalar->data.u08 = *(vx_uint8*)user_ptr; break;
    case VX_TYPE_INT16:    scalar->data.s16 = *(vx_int16*)user_ptr; break;
    case VX_TYPE_UINT16:   scalar->data.u16 = *(vx_uint16*)user_ptr; break;
    case VX_TYPE_INT32:    scalar->data.s32 = *(vx_int32*)user_ptr; break;
    case VX_TYPE_UINT32:   scalar->data.u32 = *(vx_uint32*)user_ptr; break;
    case VX_TYPE_INT64:    scalar->data.s64 = *(vx_int64*)user_ptr; break;
    case VX_TYPE_UINT64:   scalar->data.u64 = *(vx_uint64*)user_ptr; break;
#if OVX_SUPPORT_HALF_FLOAT
    case VX_TYPE_FLOAT16:  scalar->data.f16 = *(vx_float16*)user_ptr; break;
#endif
    case VX_TYPE_FLOAT32:  scalar->data.f32 = *(vx_float32*)user_ptr; break;
    case VX_TYPE_FLOAT64:  scalar->data.f64 = *(vx_float64*)user_ptr; break;
    case VX_TYPE_DF_IMAGE: scalar->data.fcc = *(vx_df_image*)user_ptr; break;
    case VX_TYPE_ENUM:     scalar->data.enm = *(vx_enum*)user_ptr; break;
    case VX_TYPE_SIZE:     scalar->data.size = *(vx_size*)user_ptr; break;
    case VX_TYPE_BOOL:     scalar->data.boolean = *(vx_bool*)user_ptr; break;

    default:
        VX_PRINT(VX_ZONE_ERROR, "some case is not covered in %s\n", __FUNCTION__);
        status = VX_ERROR_NOT_SUPPORTED;
        break;
    }

    vxPrintScalarValue(scalar);

    if (vx_false_e == ownSemPost(&scalar->base.lock))
        return VX_ERROR_NO_RESOURCES;

    ownWroteToReference(&scalar->base);

    return status;
} /* own_host_mem_to_scalar() */

VX_API_ENTRY vx_status VX_API_CALL vxCopyScalar(vx_scalar scalar, void* user_ptr, vx_enum usage, vx_enum user_mem_type)
{
    vx_status status = VX_SUCCESS;

    if (vx_false_e == ownIsValidSpecificReference(&scalar->base, VX_TYPE_SCALAR))
        return VX_ERROR_INVALID_REFERENCE;

    if (NULL == user_ptr || VX_MEMORY_TYPE_HOST != user_mem_type)
        return VX_ERROR_INVALID_PARAMETERS;


    switch (usage)
    {
    case VX_READ_ONLY:  status = own_scalar_to_host_mem(scalar, user_ptr);  break;
    case VX_WRITE_ONLY: status = own_host_mem_to_scalar(scalar, user_ptr); break;

    default:
        status = VX_ERROR_INVALID_PARAMETERS;
        break;
    }

    return status;
} /* vxCopyScalar() */

VX_API_ENTRY vx_status VX_API_CALL vxReadScalarValue(vx_scalar scalar, void *ptr)
{
    vx_status status = VX_SUCCESS;

    if (ownIsValidSpecificReference(&scalar->base,VX_TYPE_SCALAR) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (ptr == NULL)
        return VX_ERROR_INVALID_PARAMETERS;

    ownSemWait(&scalar->base.lock);
    vxPrintScalarValue(scalar);
    switch (scalar->data_type)
    {
        case VX_TYPE_CHAR:
            *(vx_char *)ptr = scalar->data.chr;
            break;
        case VX_TYPE_INT8:
            *(vx_int8 *)ptr = scalar->data.s08;
            break;
        case VX_TYPE_UINT8:
            *(vx_uint8 *)ptr = scalar->data.u08;
            break;
        case VX_TYPE_INT16:
            *(vx_int16 *)ptr = scalar->data.s16;
            break;
        case VX_TYPE_UINT16:
            *(vx_uint16 *)ptr = scalar->data.u16;
            break;
        case VX_TYPE_INT32:
            *(vx_int32 *)ptr = scalar->data.s32;
            break;
        case VX_TYPE_UINT32:
            *(vx_uint32 *)ptr = scalar->data.u32;
            break;
        case VX_TYPE_INT64:
            *(vx_int64 *)ptr = scalar->data.s64;
            break;
        case VX_TYPE_UINT64:
            *(vx_uint64 *)ptr = scalar->data.u64;
            break;
#if OVX_SUPPORT_HALF_FLOAT
        case VX_TYPE_FLOAT16:
            *(vx_float16 *)ptr = scalar->data.f16;
            break;
#endif
        case VX_TYPE_FLOAT32:
            *(vx_float32 *)ptr = scalar->data.f32;
            break;
        case VX_TYPE_FLOAT64:
            *(vx_float64 *)ptr = scalar->data.f64;
            break;
        case VX_TYPE_DF_IMAGE:
            *(vx_df_image *)ptr = scalar->data.fcc;
            break;
        case VX_TYPE_ENUM:
            *(vx_enum *)ptr = scalar->data.enm;
            break;
        case VX_TYPE_SIZE:
            *(vx_size *)ptr = scalar->data.size;
            break;
        case VX_TYPE_BOOL:
            *(vx_bool *)ptr = scalar->data.boolean;
            break;
        default:
            VX_PRINT(VX_ZONE_ERROR, "some case is not covered in %s\n", __FUNCTION__);
            status = VX_ERROR_NOT_SUPPORTED;
            break;
    }
    ownSemPost(&scalar->base.lock);
    ownReadFromReference(&scalar->base);
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxWriteScalarValue(vx_scalar scalar, const void *ptr)
{
    vx_status status = VX_SUCCESS;

    if (ownIsValidSpecificReference(&scalar->base,VX_TYPE_SCALAR) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    if (ptr == NULL)
        return VX_ERROR_INVALID_PARAMETERS;

    ownSemWait(&scalar->base.lock);
    switch (scalar->data_type)
    {
        case VX_TYPE_CHAR:
            scalar->data.chr = *(vx_char *)ptr;
            break;
        case VX_TYPE_INT8:
            scalar->data.s08 = *(vx_int8 *)ptr;
            break;
        case VX_TYPE_UINT8:
            scalar->data.u08 = *(vx_uint8 *)ptr;
            break;
        case VX_TYPE_INT16:
            scalar->data.s16 = *(vx_int16 *)ptr;
            break;
        case VX_TYPE_UINT16:
            scalar->data.u16 = *(vx_uint16 *)ptr;
            break;
        case VX_TYPE_INT32:
            scalar->data.s32 = *(vx_int32 *)ptr;
            break;
        case VX_TYPE_UINT32:
            scalar->data.u32 = *(vx_uint32 *)ptr;
            break;
        case VX_TYPE_INT64:
            scalar->data.s64 = *(vx_int64 *)ptr;
            break;
        case VX_TYPE_UINT64:
            scalar->data.u64 = *(vx_uint64 *)ptr;
            break;
#if OVX_SUPPORT_HALF_FLOAT
        case VX_TYPE_FLOAT16:
            scalar->data.f16 = *(vx_float16 *)ptr;
            break;
#endif
        case VX_TYPE_FLOAT32:
            scalar->data.f32 = *(vx_float32 *)ptr;
            break;
        case VX_TYPE_FLOAT64:
            scalar->data.f64 = *(vx_float64 *)ptr;
            break;
        case VX_TYPE_DF_IMAGE:
            scalar->data.fcc = *(vx_df_image *)ptr;
            break;
        case VX_TYPE_ENUM:
            scalar->data.enm = *(vx_enum *)ptr;
            break;
        case VX_TYPE_SIZE:
            scalar->data.size = *(vx_size *)ptr;
            break;
        case VX_TYPE_BOOL:
            scalar->data.boolean = *(vx_bool *)ptr;
            break;
        default:
            VX_PRINT(VX_ZONE_ERROR, "some case is not covered in %s\n", __FUNCTION__);
            status = VX_ERROR_NOT_SUPPORTED;
            break;
    }
    vxPrintScalarValue(scalar);
    ownSemPost(&scalar->base.lock);
    ownWroteToReference(&scalar->base);
    return status;
}
