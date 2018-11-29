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
#include "vx_lut.h"

VX_API_ENTRY vx_lut VX_API_CALL vxCreateLUT(vx_context context, vx_enum data_type, vx_size count)
{
    vx_lut_t *lut = NULL;

    if (ownIsValidContext(context) == vx_true_e)
    {
        if (data_type == VX_TYPE_UINT8)
        {
#if defined(OPENVX_STRICT_1_0)
            if (count != 256)
            {
                VX_PRINT(VX_ZONE_ERROR, "Invalid parameter to LUT\n");
                vxAddLogEntry(&context->base, VX_ERROR_INVALID_PARAMETERS, "Invalid parameter to LUT\n");
                lut = (vx_lut_t *)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
            }
            else
#endif
            {
                lut = (vx_lut_t *)ownCreateArrayInt(context, VX_TYPE_UINT8, count, vx_false_e, VX_TYPE_LUT);
                if (vxGetStatus((vx_reference)lut) == VX_SUCCESS && lut->base.type == VX_TYPE_LUT)
                {
                    lut->num_items = count;
                    lut->offset = 0;
                    ownPrintArray(lut);
                }
            }
        }
        else if (data_type == VX_TYPE_INT16)
        {
            if (!(count <= 65536))
            {
                VX_PRINT(VX_ZONE_ERROR, "Invalid parameter to LUT\n");
                vxAddLogEntry(&context->base, VX_ERROR_INVALID_PARAMETERS, "Invalid parameter to LUT\n");
                lut = (vx_lut_t *)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
            }
            else
            {
                lut = (vx_lut_t *)ownCreateArrayInt(context, VX_TYPE_INT16, count, vx_false_e, VX_TYPE_LUT);
                if (vxGetStatus((vx_reference)lut) == VX_SUCCESS && lut->base.type == VX_TYPE_LUT)
                {
                    lut->num_items = count;
                    lut->offset = (vx_uint32)(count/2);
                    ownPrintArray(lut);
                }
            }
        }
#if !defined(OPENVX_STRICT_1_0)
        else if (data_type == VX_TYPE_UINT16)
        {
            lut = (vx_lut_t *)ownCreateArrayInt(context, VX_TYPE_UINT16, count, vx_false_e, VX_TYPE_LUT);
            if (vxGetStatus((vx_reference)lut) == VX_SUCCESS && lut->base.type == VX_TYPE_LUT)
            {
                lut->num_items = count;
                lut->offset = 0;
                ownPrintArray(lut);
            }
        }
#endif
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid data type\n");
            vxAddLogEntry(&context->base, VX_ERROR_INVALID_TYPE, "Invalid data type\n");
            lut = (vx_lut_t *)ownGetErrorObject(context, VX_ERROR_INVALID_TYPE);
        }
    }

    return (vx_lut)lut;
}

VX_API_ENTRY vx_lut VX_API_CALL vxCreateVirtualLUT(vx_graph graph, vx_enum data_type, vx_size count)
{
    vx_lut_t *lut = NULL;
    vx_reference_t *gref = (vx_reference_t *)graph;

    if (ownIsValidSpecificReference(gref, VX_TYPE_GRAPH) == vx_true_e)
    {
        lut = (vx_lut_t *)vxCreateLUT(gref->context, data_type, count);
        if (vxGetStatus((vx_reference)lut) == VX_SUCCESS && lut->base.type ==  VX_TYPE_LUT)
        {
           lut->base.scope = (vx_reference_t *)graph;
           lut->base.is_virtual = vx_true_e;
        }
    }

    return (vx_lut)lut;
}


void vxDestructLUT(vx_reference ref)
{
    vx_lut_t *lut = (vx_lut_t *)ref;
    ownDestructArray((vx_reference_t *)lut);
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseLUT(vx_lut *l)
{
    return ownReleaseReferenceInt((vx_reference *)l, VX_TYPE_LUT, VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryLUT(vx_lut l, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    vx_lut_t *lut = (vx_lut_t *)l;

    if (ownIsValidSpecificReference(&lut->base, VX_TYPE_LUT) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    switch (attribute)
    {
        case VX_LUT_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
            {
                *(vx_enum *)ptr = lut->item_type;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_LUT_COUNT:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
            {
                *(vx_size *)ptr = lut->num_items;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_LUT_SIZE:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
            {
                *(vx_size *)ptr = lut->num_items * lut->item_size;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_LUT_OFFSET:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
            {
                *(vx_uint32 *)ptr = lut->offset;
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
}

VX_API_ENTRY vx_status VX_API_CALL vxAccessLUT(vx_lut l, void **ptr, vx_enum usage)
{
    vx_status status = VX_FAILURE;
    vx_lut_t *lut = (vx_lut_t *)l;
    if (ownIsValidSpecificReference(&lut->base, VX_TYPE_LUT) == vx_true_e)
    {
        status = ownAccessArrayRangeInt((vx_array_t *)l, 0, lut->num_items, NULL, ptr, usage);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxCommitLUT(vx_lut l, const void *ptr)
{
    vx_status status = VX_FAILURE;
    vx_lut_t *lut = (vx_lut_t *)l;
    if (ownIsValidSpecificReference(&lut->base, VX_TYPE_LUT) == vx_true_e)
    {
        status = ownCommitArrayRangeInt((vx_array_t *)l, 0, lut->num_items, ptr);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxCopyLUT(vx_lut l, void *user_ptr, vx_enum usage, vx_enum user_mem_type)
{
    vx_status status = VX_FAILURE;
    vx_lut_t *lut = (vx_lut_t *)l;
    if (ownIsValidSpecificReference(&lut->base, VX_TYPE_LUT) == vx_true_e)
    {
        vx_size stride = lut->item_size;
        status = ownCopyArrayRangeInt((vx_array_t *)l, 0, lut->num_items, stride, user_ptr, usage, user_mem_type);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxMapLUT(vx_lut l, vx_map_id *map_id, void **ptr, vx_enum usage, vx_enum mem_type, vx_bitfield flags)
{
    vx_status status = VX_FAILURE;
    vx_lut_t *lut = (vx_lut_t *)l;
    if (ownIsValidSpecificReference(&lut->base, VX_TYPE_LUT) == vx_true_e)
    {
        vx_size stride = lut->item_size;
        status = ownMapArrayRangeInt((vx_array_t *)lut, 0, lut->num_items, map_id, &stride, ptr, usage, mem_type, flags);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxUnmapLUT(vx_lut l, vx_map_id map_id)
{
    vx_status status = VX_FAILURE;
    vx_lut_t *lut = (vx_lut_t *)l;
    if (ownIsValidSpecificReference(&lut->base, VX_TYPE_LUT) == vx_true_e)
    {
        status = ownUnmapArrayRangeInt((vx_array_t *)l, map_id);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
    }
    return status;
}

