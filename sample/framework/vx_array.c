/* 

 * Copyright (c) 2011-2017 The Khronos Group Inc.
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
#include "vx_array.h"

/*==============================================================================
 PRIVATE INTERFACE
 =============================================================================*/

static vx_size vxArrayItemSize(vx_context context, vx_enum item_type)
{
    vx_size res = ownSizeOfType(item_type);
    vx_uint32 i = 0;
    if (res == 0ul)
    {
        for (i = 0; i < VX_INT_MAX_USER_STRUCTS; ++i)
        {
            if (context->user_structs[i].type == item_type)
            {
                res = context->user_structs[i].size;
                break;
            }
        }
    }
    return res;
}

static vx_bool vxIsValidArrayItemType(vx_context context, vx_enum item_type)
{
    vx_bool res = vx_false_e;

    if (vxArrayItemSize(context, item_type) != 0)
    {
        res = vx_true_e;
    }

    return res;
}

static vx_bool vxIsValidArray(vx_array arr)
{
    vx_bool res = vx_false_e;

    if (arr != NULL)
        res = ownIsValidSpecificReference(&arr->base, VX_TYPE_ARRAY);

    if (res == vx_true_e)
        res = vxIsValidArrayItemType(arr->base.context, arr->item_type);

    return res;
}

static void vxInitArrayMemory(vx_array arr)
{
    arr->memory.nptrs = 1;
    arr->memory.ndims = 2;

    arr->memory.dims[0][0] = (vx_uint32)arr->item_size;
    arr->memory.dims[0][1] = (vx_uint32)arr->capacity;
}

/*==============================================================================
 INTERNAL INTERFACE
 =============================================================================*/

void ownPrintArray(vx_array array)
{
    VX_PRINT(VX_ZONE_INFO, "Array:%p has %zu elements of %04x type of %zu size each.\n", array, array->capacity, array->item_type, array->item_size);
}

vx_array ownCreateArrayInt(vx_context context, vx_enum item_type, vx_size capacity, vx_bool is_virtual, vx_enum type)
{
    vx_array arr = (vx_array)ownCreateReference(context, type, VX_EXTERNAL, &context->base);
    if (vxGetStatus((vx_reference)arr) == VX_SUCCESS && arr->base.type == type)
    {
        arr->item_type = item_type;
        arr->item_size = vxArrayItemSize(context, item_type);
        arr->capacity = capacity;
        arr->base.is_virtual = is_virtual;
        vxInitArrayMemory(arr);
    }
    return arr;
}

void ownDestructArray(vx_reference ref)
{
    vx_array arr = (vx_array)ref;
    ownFreeMemory(arr->base.context, &arr->memory);
}

vx_bool ownInitVirtualArray(vx_array arr, vx_enum item_type, vx_size capacity)
{
    vx_bool res = vx_false_e;
    if ((vxIsValidArrayItemType(arr->base.context, item_type) == vx_true_e) &&
        ((arr->capacity > 0 || capacity > 0) && (capacity <= arr->capacity || arr->capacity == 0)))
    {
        if ((arr->item_type == VX_TYPE_INVALID) || (arr->item_type == item_type))
        {
            arr->item_type = item_type;
            arr->item_size = vxArrayItemSize(arr->base.context, item_type);

            if (arr->capacity == 0)
                arr->capacity = capacity;

            vxInitArrayMemory(arr);
            res = vx_true_e;
        }
    }
    return res;
}

vx_bool ownValidateArray(vx_array arr, vx_enum item_type, vx_size capacity)
{
    vx_bool res = vx_false_e;
    if ((vxIsValidArrayItemType(arr->base.context, item_type)) &&
        (arr->item_type == item_type))
    {
        /* if the required capacity is > 0 and the objects capacity is not sufficient */
        if ((capacity > 0) && (capacity > arr->capacity))
            res = vx_false_e;
        else
            res = vx_true_e;
    }
    return res;
}

vx_bool ownAllocateArray(vx_array arr)
{
    vx_bool res = vx_false_e;
    if (arr->capacity > 0)
    {
        res = ownAllocateMemory(arr->base.context, &arr->memory);
    }
    return res;
}

vx_status ownAccessArrayRangeInt(vx_array arr, vx_size start, vx_size end, vx_size *pStride, void **ptr, vx_enum usage)
{
    vx_status status = VX_FAILURE;

    /* bad parameters */
    if ((usage < VX_READ_ONLY) || (VX_READ_AND_WRITE < usage) ||
        (ptr == NULL) ||
        (start >= end) || (end > arr->num_items))
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    /* determine if virtual before checking for memory */
    if (arr->base.is_virtual == vx_true_e)
    {
        if (arr->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" array. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual array\n");
            return VX_ERROR_OPTIMIZED_AWAY;
        }
        /* framework trying to access a virtual array, this is ok. */
    }

    /* verify has not run or will not run yet. this allows this API to "touch"
     * the array to create it.
     */
    if (ownAllocateArray(arr) == vx_false_e)
    {
        return VX_ERROR_NO_MEMORY;
    }

    /* POSSIBILITIES:
     * 1.) !*ptr && RO == COPY-ON-READ (make ptr=alloc)
     * 2.) !*ptr && WO == MAP
     * 3.) !*ptr && RW == MAP
     * 4.)  *ptr && RO||RW == COPY (UNLESS MAP)
     */

    /* MAP mode */
    if (*ptr == NULL)
    {
        if ((usage == VX_WRITE_ONLY) || (usage == VX_READ_AND_WRITE))
        {
            /*-- MAP --*/
            status = VX_ERROR_NO_RESOURCES;

            /* lock the memory */
            if(ownSemWait(&arr->memory.locks[0]) == vx_true_e)
            {
                vx_size offset = start * arr->item_size;

                *ptr = &arr->memory.ptrs[0][offset];

                if (usage != VX_WRITE_ONLY)
                {
                    ownReadFromReference(&arr->base);
                }
                ownIncrementReference(&arr->base, VX_EXTERNAL);

                status = VX_SUCCESS;
            }
        }
        else
        {
            /*-- COPY-ON-READ --*/
            vx_size size = ((end - start) * arr->item_size);
            vx_uint32 a = 0u;

            vx_size *stride_save = calloc(1, sizeof(vx_size));
            *stride_save = arr->item_size;

            if (ownAddAccessor(arr->base.context, size, usage, *ptr, &arr->base, &a, stride_save) == vx_true_e)
            {
                vx_size offset;
                *ptr = arr->base.context->accessors[a].ptr;

                offset = start * arr->item_size;

                memcpy(*ptr, &arr->memory.ptrs[0][offset], size);

                ownReadFromReference(&arr->base);
                ownIncrementReference(&arr->base, VX_EXTERNAL);

                status = VX_SUCCESS;
            }
            else
            {
                status = VX_ERROR_NO_MEMORY;
                vxAddLogEntry((vx_reference)arr, status, "Failed to allocate memory for COPY-ON-READ! Size="VX_FMT_SIZE"\n", size);
            }
        }
        if ((status == VX_SUCCESS) && (pStride != NULL))
        {
            *pStride = arr->item_size;
        }
    }

    /* COPY mode */
    else
    {
        vx_size size = ((end - start) * arr->item_size);
        vx_uint32 a = 0u;

        vx_size *stride_save = calloc(1, sizeof(vx_size));
        if (pStride == NULL) {
            *stride_save = arr->item_size;
            pStride = stride_save;
        }
        else {
            *stride_save = *pStride;
        }

        if (ownAddAccessor(arr->base.context, size, usage, *ptr, &arr->base, &a, stride_save) == vx_true_e)
        {
            *ptr = arr->base.context->accessors[a].ptr;

            status = VX_SUCCESS;

            if ((usage == VX_WRITE_ONLY) || (usage == VX_READ_AND_WRITE))
            {
                if (ownSemWait(&arr->memory.locks[0]) == vx_false_e)
                {
                    status = VX_ERROR_NO_RESOURCES;
                }
            }

            if (status == VX_SUCCESS)
            {
                if (usage != VX_WRITE_ONLY)
                {
                    int i;
                    vx_uint8 *pSrc, *pDest;

                    for (i = (int)start, pDest = *ptr, pSrc = &arr->memory.ptrs[0][start * arr->item_size];
                         i < (int)end;
                         i++, pDest += *pStride, pSrc += arr->item_size)
                    {
                        memcpy(pDest, pSrc, arr->item_size);
                    }

                    ownReadFromReference(&arr->base);
                }

                ownIncrementReference(&arr->base, VX_EXTERNAL);
            }
        }
        else
        {
            status = VX_ERROR_NO_MEMORY;
            vxAddLogEntry((vx_reference)arr, status, "Failed to allocate memory for COPY-ON-READ! Size="VX_FMT_SIZE"\n", size);
        }
    }

    return status;
}

vx_status ownCommitArrayRangeInt(vx_array arr, vx_size start, vx_size end, const void *ptr)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;

    vx_bool external = vx_true_e; // assume that it was an allocated buffer

    if ((ptr == NULL) ||
        (start > end) || (end > arr->num_items))
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    /* determine if virtual before checking for memory */
    if (arr->base.is_virtual == vx_true_e)
    {
        if (arr->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" array. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual array\n");
            return VX_ERROR_OPTIMIZED_AWAY;
        }
        /* framework trying to access a virtual array, this is ok. */
    }

    /* VARIABLES:
     * 1.) ZERO_AREA
     * 2.) CONSTANT - independant
     * 3.) INTERNAL - independant of area
     * 4.) EXTERNAL - dependant on area (do nothing on zero, determine on non-zero)
     * 5.) !INTERNAL && !EXTERNAL == MAPPED
     */

    {
        /* check to see if the range is zero area */
        vx_bool zero_area = (end == 0) ? vx_true_e : vx_false_e;
        vx_uint32 index = UINT32_MAX; // out of bounds, if given to remove, won't do anything
        vx_bool internal = ownFindAccessor(arr->base.context, ptr, &index);

        if (zero_area == vx_false_e)
        {
            /* this could be a write-back */
            if (internal == vx_true_e && arr->base.context->accessors[index].usage == VX_READ_ONLY)
            {
                /* this is a buffer that we allocated on behalf of the user and now they are done. Do nothing else*/
                ownRemoveAccessor(arr->base.context, index);
            }
            else
            {
                vx_uint8 *beg_ptr = arr->memory.ptrs[0];
                vx_uint8 *end_ptr = &beg_ptr[arr->item_size * arr->num_items];

                if ((beg_ptr <= (vx_uint8 *)ptr) && ((vx_uint8 *)ptr < end_ptr))
                {
                    /* the pointer in contained in the array, so it was mapped, thus
                     * there's nothing else to do. */
                    external = vx_false_e;
                }

                if (external == vx_true_e || internal == vx_true_e)
                {
                    /* the pointer was not mapped, copy. */
                    vx_size offset = start * arr->item_size;
                    vx_size len = (end - start) * arr->item_size;

                    if (internal == vx_true_e)
                    {
                        vx_size stride = *(vx_size *)arr->base.context->accessors[index].extra_data;

                        if (stride == arr->item_size) {
                            memcpy(&beg_ptr[offset], ptr, len);
                        }
                        else {
                            int i;
                            const vx_uint8 *pSrc; vx_uint8 *pDest;

                            for (i = (int)start, pSrc = ptr, pDest= &beg_ptr[offset];
                                 i < (int)end;
                                 i++, pSrc += stride, pDest += arr->item_size)
                            {
                                memcpy(pDest, pSrc, arr->item_size);
                            }
                        }

                        /* a write only or read/write copy */
                        ownRemoveAccessor(arr->base.context, index);
                    }
                    else {
                        memcpy(&beg_ptr[offset], ptr, len);
                    }
                }

                ownWroteToReference(&arr->base);
            }

            ownSemPost(&arr->memory.locks[0]);

            status = VX_SUCCESS;
        }
        else
        {
            /* could be RO|WO|RW where they decided not to commit anything. */
            if (internal == vx_true_e) // RO
            {
                ownRemoveAccessor(arr->base.context, index);
            }
            else // RW|WO
            {
                ownSemPost(&arr->memory.locks[0]);
            }

            status = VX_SUCCESS;
        }

        ownDecrementReference(&arr->base, VX_EXTERNAL);
    }

    return status;
}

vx_status ownCopyArrayRangeInt(vx_array arr, vx_size start, vx_size end, vx_size stride, void *ptr, vx_enum usage, vx_enum mem_type)
{
    vx_status status = VX_FAILURE;
    (void)mem_type;

    /* bad parameters */
    if (((usage != VX_READ_ONLY) && (VX_WRITE_ONLY != usage)) ||
         (ptr == NULL) || (stride < arr->item_size) ||
         (start >= end) || (end > arr->num_items))
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    /* determine if virtual before checking for memory */
    if (arr->base.is_virtual == vx_true_e)
    {
        if (arr->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" array. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual array\n");
            return VX_ERROR_OPTIMIZED_AWAY;
        }
        /* framework trying to access a virtual array, this is ok. */
    }

    /* verify has not run or will not run yet. this allows this API to "touch"
     * the array to create it.
     */
    if (ownAllocateArray(arr) == vx_false_e)
    {
        return VX_ERROR_NO_MEMORY;
    }

    vx_size offset = start * arr->item_size;
    if (usage == VX_READ_ONLY)
    {
        VX_PRINT(VX_ZONE_ARRAY, "CopyArrayRange from "VX_FMT_REF" to ptr %p from %u to %u\n", arr, ptr, start, end);

        vx_uint8 *pSrc = (vx_uint8 *)&arr->memory.ptrs[0][offset];
        vx_uint8 *pDst = (vx_uint8 *)ptr;
        if (stride == arr->item_size)
        {
            vx_size size = (end - start) * arr->item_size;
            memcpy(pDst, pSrc, size);
        }
        else
        {
            /* The source is not compact, we need to copy per element */
            for (vx_size i = start; i < end; i++)
            {
                memcpy(pDst, pSrc, arr->item_size);
                pDst += stride;
                pSrc += arr->item_size;
            }
        }

        ownReadFromReference(&arr->base);
        status = VX_SUCCESS;
    }
    else
    {
        VX_PRINT(VX_ZONE_ARRAY, "CopyArrayRange from ptr %p to "VX_FMT_REF" from %u to %u\n", arr, ptr, start, end);

        if (ownSemWait(&arr->memory.locks[0]) == vx_true_e)
        {
            vx_uint8 *pSrc = (vx_uint8 *)ptr;
            vx_uint8 *pDst = (vx_uint8 *)&arr->memory.ptrs[0][offset];
            if (stride == arr->item_size)
            {
                vx_size size = (end - start) * arr->item_size;
                memcpy(pDst, pSrc, size);
            }
            else
            {
                /* The source is not compact, we need to copy per element */
                for (vx_size i = start; i < end; i++)
                {
                    memcpy(pDst, pSrc, arr->item_size);
                    pDst += arr->item_size;
                    pSrc += stride;
                }
            }

            ownWroteToReference(&arr->base);
            ownSemPost(&arr->memory.locks[0]);
            status = VX_SUCCESS;
        }
        else
        {
            status = VX_ERROR_NO_RESOURCES;
        }
    }

    return status;
}

vx_status ownMapArrayRangeInt(vx_array arr, vx_size start, vx_size end, vx_map_id *map_id, vx_size *stride,
                             void **ptr, vx_enum usage, vx_enum mem_type, vx_uint32 flags)
{
    vx_status status = VX_FAILURE;

    /* bad parameters */
    if ((usage < VX_READ_ONLY) || (VX_READ_AND_WRITE < usage) ||
        (ptr == NULL) || (stride == NULL) ||
        (start >= end) || (end > arr->num_items))
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    /* determine if virtual before checking for memory */
    if (arr->base.is_virtual == vx_true_e)
    {
        if (arr->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" array. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual array\n");
            return VX_ERROR_OPTIMIZED_AWAY;
        }
        /* framework trying to access a virtual array, this is ok. */
    }

    /* verify has not run or will not run yet. this allows this API to "touch"
     * the array to create it.
     */
    if (ownAllocateArray(arr) == vx_false_e)
    {
        return VX_ERROR_NO_MEMORY;
    }

    VX_PRINT(VX_ZONE_ARRAY, "MapArrayRange from "VX_FMT_REF" to ptr %p from %u to %u\n", arr, *ptr, start, end);

    vx_memory_map_extra extra;
    extra.array_data.start = start;
    extra.array_data.end = end;
    vx_uint8 *buf = NULL;
    vx_size size = (end - start) * arr->item_size;
    if (ownMemoryMap(arr->base.context, (vx_reference)arr, size, usage, mem_type, flags, &extra, (void **)&buf, map_id) == vx_true_e)
    {
        if (VX_READ_ONLY == usage || VX_READ_AND_WRITE == usage)
        {
            if (ownSemWait(&arr->memory.locks[0]) == vx_true_e)
            {
                *stride = arr->item_size;

                vx_uint32 offset = (vx_uint32)(start * arr->item_size);
                vx_uint8 *pSrc = (vx_uint8 *)&arr->memory.ptrs[0][offset];
                vx_uint8 *pDst = (vx_uint8 *)buf;
                memcpy(pDst, pSrc, size);

                *ptr = buf;
                ownIncrementReference(&arr->base, VX_EXTERNAL);
                ownSemPost(&arr->memory.locks[0]);

                status = VX_SUCCESS;
            }
            else
            {
                status = VX_ERROR_NO_RESOURCES;
            }
        }
        else
        {
            /* write only mode */
            *stride = arr->item_size;
            *ptr = buf;
            ownIncrementReference(&arr->base, VX_EXTERNAL);
            status = VX_SUCCESS;
        }
    }
    else
    {
        status = VX_FAILURE;
    }

    return status;
}

vx_status ownUnmapArrayRangeInt(vx_array arr, vx_map_id map_id)
{
    vx_status status = VX_FAILURE;

    /* determine if virtual before checking for memory */
    if (arr->base.is_virtual == vx_true_e)
    {
        if (arr->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" array. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual array\n");
            return VX_ERROR_OPTIMIZED_AWAY;
        }
        /* framework trying to access a virtual array, this is ok. */
    }

    /* bad parameters */
    if (ownFindMemoryMap(arr->base.context, (vx_reference)arr, map_id) != vx_true_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid parameters to unmap array range\n");
        return VX_ERROR_INVALID_PARAMETERS;
    }

    VX_PRINT(VX_ZONE_ARRAY, "UnmapArrayRange from "VX_FMT_REF"\n", arr);

    vx_context context = arr->base.context;
    vx_memory_map_t* map = &context->memory_maps[map_id];
    if (map->used && map->ref == (vx_reference)arr)
    {
        vx_size start = map->extra.array_data.start;
        vx_size end = map->extra.array_data.end;
        if (VX_WRITE_ONLY == map->usage || VX_READ_AND_WRITE == map->usage)
        {
            if (ownSemWait(&arr->memory.locks[0]) == vx_true_e)
            {
                vx_uint32 offset = (vx_uint32)(start * arr->item_size);
                vx_uint8 *pSrc = (vx_uint8 *)map->ptr;
                vx_uint8 *pDst = (vx_uint8 *)&arr->memory.ptrs[0][offset];
                vx_size size = (end - start) * arr->item_size;
                memcpy(pDst, pSrc, size);

                ownMemoryUnmap(context, (vx_uint32)map_id);
                ownDecrementReference(&arr->base, VX_EXTERNAL);
                ownSemPost(&arr->memory.locks[0]);
                status = VX_SUCCESS;
            }
            else
            {
                status = VX_ERROR_NO_RESOURCES;
            }
        }
        else
        {
            /* rean only mode */
            ownMemoryUnmap(arr->base.context, (vx_uint32)map_id);
            ownDecrementReference(&arr->base, VX_EXTERNAL);
            status = VX_SUCCESS;
        }
    }
    else
    {
        status = VX_FAILURE;
    }

    return status;
}

/*==============================================================================
 PUBLIC INTERFACE
 =============================================================================*/

VX_API_ENTRY vx_array VX_API_CALL vxCreateArray(vx_context context, vx_enum item_type, vx_size capacity)
{
    vx_array arr = NULL;

    if (ownIsValidContext(context) == vx_true_e)
    {
        if ( (vxIsValidArrayItemType(context, item_type) == vx_true_e) &&
             (capacity > 0))
        {
            arr = (vx_array)ownCreateArrayInt(context, item_type, capacity, vx_false_e, VX_TYPE_ARRAY);

            if (arr == NULL)
            {
                arr = (vx_array)ownGetErrorObject(context, VX_ERROR_NO_MEMORY);
            }
        }
        else
        {
            arr = (vx_array)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
        }
    }

    return arr;
}

VX_API_ENTRY vx_array VX_API_CALL vxCreateVirtualArray(vx_graph graph, vx_enum item_type, vx_size capacity)
{
    vx_array arr = NULL;

    if (ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) == vx_true_e)
    {
        if (((vxIsValidArrayItemType(graph->base.context, item_type) == vx_true_e) || item_type == VX_TYPE_INVALID))
        {
            arr = (vx_array)ownCreateArrayInt(graph->base.context, item_type, capacity, vx_true_e, VX_TYPE_ARRAY);

            if (arr && arr->base.type == VX_TYPE_ARRAY)
            {
                arr->base.scope = (vx_reference_t *)graph;
            }
            else
            {
                arr = (vx_array)ownGetErrorObject(graph->base.context, VX_ERROR_NO_MEMORY);
            }
        }
        else
        {
            arr = (vx_array)ownGetErrorObject(graph->base.context, VX_ERROR_INVALID_PARAMETERS);
        }
    }

    return arr;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseArray(vx_array *a)
{
    /* NULL means standard destructor */
    return ownReleaseReferenceInt((vx_reference_t **)a, VX_TYPE_ARRAY, VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryArray(vx_array arr, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (vxIsValidArray(arr) == vx_true_e)
    {
        status = VX_SUCCESS;
        switch (attribute)
        {
            case VX_ARRAY_ITEMTYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    *(vx_enum *)ptr = arr->item_type;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;

            case VX_ARRAY_NUMITEMS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = arr->num_items;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;

            case VX_ARRAY_CAPACITY:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = arr->capacity;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;

            case VX_ARRAY_ITEMSIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    *(vx_size *)ptr = arr->item_size;
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
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxAddArrayItems(vx_array arr, vx_size count, const void *ptr, vx_size stride)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (vxIsValidArray(arr) == vx_true_e)
    {
        status = VX_ERROR_NO_MEMORY;

        if(ownAllocateArray(arr) == vx_true_e)
        {
            status = VX_ERROR_INVALID_PARAMETERS;

            if ((count > 0) && (ptr != NULL) && (stride >= arr->item_size))
            {
                status = VX_FAILURE;

                if (arr->num_items + count <= arr->capacity)
                {
                    vx_size offset = arr->num_items * arr->item_size;
                    vx_uint8 *dst_ptr = &arr->memory.ptrs[0][offset];

                    vx_size i;
                    for (i = 0; i < count; ++i)
                    {
                        vx_uint8 *tmp = (vx_uint8 *)ptr;
                        memcpy(&dst_ptr[i * arr->item_size], &tmp[i * stride], arr->item_size);
                    }

                    arr->num_items += count;
                    ownWroteToReference(&arr->base);

                    status = VX_SUCCESS;
                }
            }
        }
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxTruncateArray(vx_array arr, vx_size new_num_items)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (vxIsValidArray(arr) == vx_true_e)
    {
        status = VX_ERROR_INVALID_PARAMETERS;

        if (new_num_items <= arr->num_items)
        {
            arr->num_items = new_num_items;
            ownWroteToReference(&arr->base);

            status = VX_SUCCESS;
        }
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxAccessArrayRange(vx_array arr, vx_size start, vx_size end, vx_size *stride, void **ptr, vx_enum usage)
{
    vx_status status = VX_FAILURE;
    /* bad references */
    if (vxIsValidArray(arr) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid array!\n");
        return VX_ERROR_INVALID_REFERENCE;
    }

    /* bad parameters */
    if (stride == NULL)
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    status = ownAccessArrayRangeInt(arr, start, end, stride, ptr, usage);

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxCommitArrayRange(vx_array arr, vx_size start, vx_size end, const void *ptr)
{
    if (vxIsValidArray(arr) == vx_false_e)
    {
        return VX_ERROR_INVALID_REFERENCE;
    }
    return ownCommitArrayRangeInt(arr, start, end, ptr);
}

VX_API_ENTRY vx_status VX_API_CALL vxCopyArrayRange(vx_array arr, vx_size start, vx_size end, vx_size stride,
                                                    void *ptr, vx_enum usage, vx_enum mem_type)
{
    vx_status status = VX_FAILURE;
    /* bad references */
    if (vxIsValidArray(arr) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid array!\n");
        return VX_ERROR_INVALID_REFERENCE;
    }

    status = ownCopyArrayRangeInt(arr, start, end, stride, ptr, usage, mem_type);

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxMapArrayRange(vx_array arr, vx_size start, vx_size end, vx_map_id *map_id, vx_size *stride,
                                                   void **ptr, vx_enum usage, vx_enum mem_type, vx_uint32 flags)
{
    vx_status status = VX_FAILURE;
    /* bad references */
    if (vxIsValidArray(arr) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid array!\n");
        return VX_ERROR_INVALID_REFERENCE;
    }

    status = ownMapArrayRangeInt(arr, start, end, map_id, stride, ptr, usage, mem_type, flags);

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxUnmapArrayRange(vx_array arr, vx_map_id map_id)
{
    vx_status status = VX_FAILURE;
    /* bad references */
    if (vxIsValidArray(arr) == vx_false_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid array!\n");
        return VX_ERROR_INVALID_REFERENCE;
    }

    status = ownUnmapArrayRangeInt(arr, map_id);

    return status;
}
