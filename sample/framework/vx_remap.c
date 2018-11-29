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
#include "vx_remap.h"

VX_API_ENTRY vx_remap VX_API_CALL vxCreateRemap(vx_context context,
                              vx_uint32 src_width, vx_uint32 src_height,
                              vx_uint32 dst_width, vx_uint32 dst_height)
{
    vx_remap remap = NULL;

    if (ownIsValidContext(context) == vx_true_e)
    {
        if (src_width != 0 && src_height != 0 && dst_width != 0 && dst_height != 0)
        {
            remap = (vx_remap)ownCreateReference(context, VX_TYPE_REMAP, VX_EXTERNAL, &context->base);
            if (vxGetStatus((vx_reference)remap) == VX_SUCCESS && remap->base.type == VX_TYPE_REMAP)
            {
                remap->src_width = src_width;
                remap->src_height = src_height;
                remap->dst_width = dst_width;
                remap->dst_height = dst_height;
                remap->memory.ndims = 3;
                remap->memory.nptrs = 1;
                remap->memory.dims[0][VX_DIM_C] = 2; // 2 "channels" of f32
                remap->memory.dims[0][VX_DIM_X] = dst_width;
                remap->memory.dims[0][VX_DIM_Y] = dst_height;
                remap->memory.strides[0][VX_DIM_C] = sizeof(vx_float32);
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid parameters to remap\n");
            vxAddLogEntry(&context->base, VX_ERROR_INVALID_PARAMETERS, "Invalid parameters to remap\n");
            remap = (vx_remap_t *)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
        }
    }

    return remap;
}

void ownDestructRemap(vx_reference ref)
{
    vx_remap remap = (vx_remap_t *)ref;
    ownFreeMemory(remap->base.context, &remap->memory);
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseRemap(vx_remap *r)
{
    return ownReleaseReferenceInt((vx_reference *)r, VX_TYPE_REMAP, VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryRemap(vx_remap remap, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidSpecificReference(&remap->base, VX_TYPE_REMAP) == vx_false_e)
        return VX_ERROR_INVALID_REFERENCE;

    switch (attribute)
    {
        case VX_REMAP_SOURCE_WIDTH:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
            {
                *(vx_uint32 *)ptr = remap->src_width;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REMAP_SOURCE_HEIGHT:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
            {
                *(vx_uint32 *)ptr = remap->src_height;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REMAP_DESTINATION_WIDTH:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
            {
                *(vx_uint32 *)ptr = remap->dst_width;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_REMAP_DESTINATION_HEIGHT:
            if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
            {
                *(vx_uint32 *)ptr = remap->dst_height;
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

VX_API_ENTRY vx_status VX_API_CALL vxSetRemapPoint(vx_remap remap, vx_uint32 dst_x, vx_uint32 dst_y,
                                 vx_float32 src_x, vx_float32 src_y)
{
    vx_status status = VX_FAILURE;
    if ((ownIsValidSpecificReference(&remap->base, VX_TYPE_REMAP) == vx_true_e) &&
         (ownAllocateMemory(remap->base.context, &remap->memory) == vx_true_e))
    {
        if ((dst_x < remap->dst_width) &&
            (dst_y < remap->dst_height))
        {
            vx_float32 *coords[] = {
                 ownFormatMemoryPtr(&remap->memory, 0, dst_x, dst_y, 0),
                 ownFormatMemoryPtr(&remap->memory, 1, dst_x, dst_y, 0),
            };
            *coords[0] = src_x;
            *coords[1] = src_y;
            ownWroteToReference(&remap->base);
            status = VX_SUCCESS;
            VX_PRINT(VX_ZONE_INFO, "SetRemapPoint %ux%u to %f,%f\n", dst_x, dst_y, src_x, src_y);
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid source or destintation values!\n");
            status = VX_ERROR_INVALID_VALUE;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxGetRemapPoint(vx_remap remap, vx_uint32 dst_x, vx_uint32 dst_y,
                                 vx_float32 *src_x, vx_float32 *src_y)
{
    vx_status status = VX_FAILURE;
    if (ownIsValidSpecificReference(&remap->base, VX_TYPE_REMAP) == vx_true_e)
    {
        if ((dst_x < remap->dst_width) &&
            (dst_y < remap->dst_height))
        {
            vx_float32 *coords[] = {
                 ownFormatMemoryPtr(&remap->memory, 0, dst_x, dst_y, 0),
                 ownFormatMemoryPtr(&remap->memory, 1, dst_x, dst_y, 0),
            };
            *src_x = *coords[0];
            *src_y = *coords[1];
            remap->base.read_count++;
            status = VX_SUCCESS;

            VX_PRINT(VX_ZONE_INFO, "GetRemapPoint dst[%u,%u] to src[%f,%f]\n", dst_x, dst_y, src_x, src_y);
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid source or destintation values!\n");
            status = VX_ERROR_INVALID_VALUE;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

VX_API_ENTRY vx_remap VX_API_CALL vxCreateVirtualRemap(vx_graph graph,
                              vx_uint32 src_width,
                              vx_uint32 src_height,
                              vx_uint32 dst_width,
                              vx_uint32 dst_height)
{
    vx_remap remap = NULL;
    vx_reference_t *gref = (vx_reference_t *)graph;

    if (ownIsValidSpecificReference(gref, VX_TYPE_GRAPH) == vx_true_e)
    {
        remap = vxCreateRemap(gref->context, src_width, src_height, dst_width, dst_height);
        if (vxGetStatus((vx_reference)remap) == VX_SUCCESS && remap->base.type == VX_TYPE_REMAP)
        {
            remap->base.scope = (vx_reference_t *)graph;
            remap->base.is_virtual = vx_true_e;
        }
    }
    /* else, the graph is invalid, we can't get any context and then error object */
    return remap;
}

static vx_bool ownIsValidRemap(vx_remap remap)
{
    if (ownIsValidSpecificReference(&remap->base, VX_TYPE_REMAP) == vx_true_e)
    {
        return vx_true_e;
    }
    else
    {
        return vx_false_e;
    }
}

static vx_status vxSetCoordValue(vx_remap remap, vx_uint32 dst_x, vx_uint32 dst_y,
                                 vx_float32 src_x, vx_float32 src_y)
{
    vx_status status = VX_FAILURE;
    if ((ownIsValidSpecificReference(&remap->base, VX_TYPE_REMAP) == vx_true_e) &&
         (ownAllocateMemory(remap->base.context, &remap->memory) == vx_true_e))
    {
        if ((dst_x < remap->dst_width) &&
            (dst_y < remap->dst_height))
        {
            vx_float32 *coords[] = {
                 ownFormatMemoryPtr(&remap->memory, 0, dst_x, dst_y, 0),
                 ownFormatMemoryPtr(&remap->memory, 1, dst_x, dst_y, 0),
            };
            *coords[0] = src_x;
            *coords[1] = src_y;
            ownWroteToReference(&remap->base);
            status = VX_SUCCESS;
            VX_PRINT(VX_ZONE_INFO, "SetCoordValue %ux%u to %f,%f\n", dst_x, dst_y, src_x, src_y);
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid source or destintation values!\n");
            status = VX_ERROR_INVALID_VALUE;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

static vx_status vxGetCoordValue(vx_remap remap, vx_uint32 dst_x, vx_uint32 dst_y,
                                 vx_float32 *src_x, vx_float32 *src_y)
{
    vx_status status = VX_FAILURE;
    if (ownIsValidSpecificReference(&remap->base, VX_TYPE_REMAP) == vx_true_e)
    {
        if ((dst_x < remap->dst_width) &&
            (dst_y < remap->dst_height))
        {
            vx_float32 *coords[] = {
                 ownFormatMemoryPtr(&remap->memory, 0, dst_x, dst_y, 0),
                 ownFormatMemoryPtr(&remap->memory, 1, dst_x, dst_y, 0),
            };
            *src_x = *coords[0];
            *src_y = *coords[1];
            remap->base.read_count++;
            status = VX_SUCCESS;

            VX_PRINT(VX_ZONE_INFO, "GetCoordValue dst[%u,%u] to src[%f,%f]\n", dst_x, dst_y, src_x, src_y);
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid source or destintation values!\n");
            status = VX_ERROR_INVALID_VALUE;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Not a valid object!\n");
        status = VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxCopyRemapPatch(vx_remap remap,
                                                    const vx_rectangle_t *rect,
                                                    vx_size user_stride_y,
                                                    void * user_ptr,
                                                    vx_enum user_coordinate_type,
                                                    vx_enum usage,
                                                    vx_enum user_mem_type)
{
    vx_status status = VX_FAILURE;
    vx_uint32 start_x = rect ? rect->start_x : 0u;
    vx_uint32 start_y = rect ? rect->start_y : 0u;
    vx_uint32 end_x = rect ? rect->end_x : 0u;
    vx_uint32 end_y = rect ? rect->end_y : 0u;
    vx_bool zero_area = ((((end_x - start_x) == 0) || ((end_y - start_y) == 0)) ? vx_true_e : vx_false_e);

    /* bad parameters */
    if ( ((VX_READ_ONLY != usage) && (VX_WRITE_ONLY != usage)) ||
         (rect == NULL) || (remap == NULL) || (user_ptr == NULL) )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* more bad parameters */
    if( (user_stride_y < sizeof(vx_coordinates2df_t)*(rect->end_x - rect->start_x)) ||
        (user_coordinate_type != VX_TYPE_COORDINATES2DF))
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* more bad parameters */
    if (user_mem_type != VX_MEMORY_TYPE_HOST && user_mem_type != VX_MEMORY_TYPE_NONE)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* bad references */
    if ( ownIsValidRemap(remap) == vx_false_e )
    {
        status = VX_ERROR_INVALID_REFERENCE;
        goto exit;
    }

    /* determine if virtual before checking for memory */
    if (remap->base.is_virtual == vx_true_e)
    {
        if (remap->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" remap. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual remap\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            goto exit;
        }
        /* framework trying to access a virtual remap, this is ok. */
    }

    /* more bad parameters */
    if (zero_area == vx_false_e &&
        ((0 >= remap->memory.nptrs) ||
         (start_x >= end_x) ||
         (start_y >= end_y)))
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    vx_size stride = user_stride_y / sizeof(vx_coordinates2df_t);

    if (usage == VX_READ_ONLY)
    {
        /* Copy from remap (READ) mode */
        vx_coordinates2df_t *ptr = user_ptr;
        vx_uint32 i;
        vx_uint32 j;
        for (i = start_y; i < end_y; i++)
        {
            for (j = start_x; j < end_x; j++)
            {
                vx_coordinates2df_t *coord_ptr = &(ptr[i * stride + j]);
                status = vxGetCoordValue(remap, j, i, &coord_ptr->x, &coord_ptr->y);
                if(status != VX_SUCCESS)
                {
                    goto exit;
                }

            }
        }
    }
    else
    {
        /* Copy to remap (WRITE) mode */
        vx_coordinates2df_t *ptr = user_ptr;
        vx_uint32 i;
        vx_uint32 j;
        for (i = start_y; i < end_y; i++)
        {
            for (j = start_x; j < end_x; j++)
            {
                vx_coordinates2df_t *coord_ptr = &(ptr[i * stride + j]);
                status = vxSetCoordValue(remap, j, i, coord_ptr->x, coord_ptr->y);
                if(status != VX_SUCCESS)
                {
                    goto exit;
                }

            }
        }
    }
    status = VX_SUCCESS;
exit:
    VX_PRINT(VX_ZONE_API, "returned %d\n", status);
    return status;
}


VX_API_ENTRY vx_status VX_API_CALL vxMapRemapPatch(vx_remap remap,
                                                   const vx_rectangle_t *rect,
                                                   vx_map_id *map_id,
                                                   vx_size *stride_y,
                                                   void **ptr,
                                                   vx_enum coordinate_type,
                                                   vx_enum usage,
                                                   vx_enum mem_type)
{
    vx_uint32 start_x = rect ? rect->start_x : 0u;
    vx_uint32 start_y = rect ? rect->start_y : 0u;
    vx_uint32 end_x   = rect ? rect->end_x : 0u;
    vx_uint32 end_y   = rect ? rect->end_y : 0u;
    vx_bool zero_area = ((((end_x - start_x) == 0) || ((end_y - start_y) == 0)) ? vx_true_e : vx_false_e);
    vx_status status = VX_FAILURE;

    /* bad parameters */
    if ( (rect == NULL) || (map_id == NULL) || (remap == NULL) || (ptr == NULL) )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* more bad parameters */
    if(coordinate_type != VX_TYPE_COORDINATES2DF)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* bad references */
    if (ownIsValidRemap(remap) == vx_false_e)
    {
        status = VX_ERROR_INVALID_REFERENCE;
        goto exit;
    }

    /* determine if virtual before checking for memory */
    if (remap->base.is_virtual == vx_true_e)
    {
        if (remap->base.is_accessible == vx_false_e)
        {
            /* User tried to access a "virtual" remap. */
            VX_PRINT(VX_ZONE_ERROR, "Can not access a virtual remap\n");
            status = VX_ERROR_OPTIMIZED_AWAY;
            goto exit;
        }
        /* framework trying to access a virtual remap, this is ok. */
    }

    /* more bad parameters */
    if (zero_area == vx_false_e &&
        ((0 >= remap->memory.nptrs) ||
        (start_x >= end_x) ||
        (start_y >= end_y)))
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    /* MAP mode */
    vx_memory_map_extra extra;
    extra.image_data.plane_index = 0;
    extra.image_data.rect = *rect;

    vx_uint32 flags = 0;
    vx_uint8 *buf = NULL;
    vx_size stride = (end_x - start_x);
    vx_size size = (stride * (end_y - start_y)) * sizeof(vx_coordinates2df_t);
    vx_size user_stride_y = stride * sizeof(vx_coordinates2df_t);

    if (ownMemoryMap(remap->base.context, (vx_reference)remap, size, usage, mem_type, flags, &extra, (void **)&buf, map_id) == vx_true_e)
    {
        if (VX_READ_ONLY == usage || VX_READ_AND_WRITE == usage)
        {

            if (ownSemWait(&remap->memory.locks[0]) == vx_true_e)
            {
                *stride_y = user_stride_y;

                vx_coordinates2df_t *buf_ptr = (vx_coordinates2df_t *)buf;
                vx_uint32 i;
                vx_uint32 j;
                for (i = start_y; i < end_y; i++)
                {
                    for (j = start_x; j < end_x; j++)
                    {
                        vx_coordinates2df_t *coord_ptr = &(buf_ptr[i * stride + j]);
                        status = vxGetCoordValue(remap, j, i, &coord_ptr->x, &coord_ptr->y);
                        if(status != VX_SUCCESS)
                        {
                            goto exit;
                        }
                    }
                }

                *ptr = buf;
                ownIncrementReference(&remap->base, VX_EXTERNAL);
                ownSemPost(&remap->memory.locks[0]);

                status = VX_SUCCESS;
            }
            else
            {
                status = VX_ERROR_NO_RESOURCES;
                goto exit;
            }
        }
        else
        {
            /* write only mode */
            *stride_y = user_stride_y;
            *ptr = buf;
            ownIncrementReference(&remap->base, VX_EXTERNAL);
            status = VX_SUCCESS;
        }
    }
    else
    {
        status = VX_FAILURE;
        goto exit;
    }
exit:
    VX_PRINT(VX_ZONE_API, "return %d\n", status);
    return status;
}


VX_API_ENTRY vx_status VX_API_CALL vxUnmapRemapPatch(vx_remap remap, vx_map_id map_id)
{
    vx_status status = VX_FAILURE;

    /* bad references */
    if (ownIsValidRemap(remap) == vx_false_e)
    {
        status = VX_ERROR_INVALID_REFERENCE;
        goto exit;
    }

    /* bad parameters */
    if (ownFindMemoryMap(remap->base.context, (vx_reference)remap, map_id) != vx_true_e)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        goto exit;
    }

    vx_context context = remap->base.context;
    vx_memory_map_t* map = &context->memory_maps[map_id];
    if (map->used && map->ref == (vx_reference)remap)
    {
        vx_rectangle_t rect = map->extra.image_data.rect;
        if (VX_WRITE_ONLY == map->usage || VX_READ_AND_WRITE == map->usage)
        {
            vx_size stride = (rect.end_x - rect.start_x);
            vx_coordinates2df_t *ptr = (vx_coordinates2df_t *)map->ptr;
            vx_uint32 i;
            vx_uint32 j;
            for (i = rect.start_y; i < rect.end_y; i++)
            {
                for (j = rect.start_x; j < rect.end_x; j++)
                {
                    vx_coordinates2df_t *coord_ptr = &(ptr[i * stride + j]);
                    status = vxSetCoordValue(remap, j, i, coord_ptr->x, coord_ptr->y);
                    if(status != VX_SUCCESS)
                    {
                        goto exit;
                    }
                }
            }

            ownMemoryUnmap(context, (vx_uint32)map_id);
            ownDecrementReference(&remap->base, VX_EXTERNAL);
            status = VX_SUCCESS;
        }
        else
        {
            /* rean only mode */
            ownMemoryUnmap(remap->base.context, (vx_uint32)map_id);
            ownDecrementReference(&remap->base, VX_EXTERNAL);
            status = VX_SUCCESS;
        }
    }
    else
    {
        status = VX_FAILURE;
        goto exit;
    }
exit:
    VX_PRINT(VX_ZONE_API, "return %d\n", status);
    return status;
}
