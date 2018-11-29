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

#include "debug_k.h"
#include <string.h>


// nodeless version of the CopyImage kernel
vx_status ownCopyImage(vx_image input, vx_image output)
{
    vx_uint32 p = 0;
    vx_uint32 y = 0;
    vx_uint32 len = 0;
    vx_size planes = 0;
    void* src;
    void* dst;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS; // assume success until an error occurs.

    status |= vxGetValidRegionImage(input, &rect);

    status |= vxQueryImage(input, VX_IMAGE_PLANES, &planes, sizeof(planes));

    for (p = 0; p < planes && status == VX_SUCCESS; p++)
    {
        status = VX_SUCCESS;
        src = NULL;
        dst = NULL;

        status |= vxMapImagePatch(input, &rect, p, &src_map_id, &src_addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
        status |= vxMapImagePatch(output, &rect, p, &dst_map_id, &dst_addr, &dst, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        for (y = 0; y < src_addr.dim_y && status == VX_SUCCESS; y += src_addr.step_y)
        {
            /*
             * in the case where the secondary planes are subsampled, the
             * scale will skip over the lines that are repeated.
             */
            vx_uint8* srcp = vxFormatImagePatchAddress2d(src, 0, y, &src_addr);
            vx_uint8* dstp = vxFormatImagePatchAddress2d(dst, 0, y, &dst_addr);

            len = (src_addr.stride_x * src_addr.dim_x * src_addr.scale_x) / VX_SCALE_UNITY;

            memcpy(dstp, srcp, len);
        }

        if (status == VX_SUCCESS)
        {
            status |= vxUnmapImagePatch(input, src_map_id);
            status |= vxUnmapImagePatch(output, dst_map_id);
        }
    }

    return status;
} /* ownCopyImage() */


// nodeless version of the CopyArray kernel
vx_status ownCopyArray(vx_array src, vx_array dst)
{
    vx_size src_num_items = 0;
    vx_size dst_capacity = 0;
    vx_size src_stride = 0;
    void* srcp = NULL;
    vx_map_id map_id = 0;
    vx_status status = VX_SUCCESS; // assume success until an error occurs.

    status = VX_SUCCESS;

    status |= vxQueryArray(src, VX_ARRAY_NUMITEMS, &src_num_items, sizeof(src_num_items));
    status |= vxQueryArray(dst, VX_ARRAY_CAPACITY, &dst_capacity, sizeof(dst_capacity));

    if (status == VX_SUCCESS)
    {
        status |= vxMapArrayRange(src, 0, src_num_items, &map_id, &src_stride, &srcp, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

        if (src_num_items <= dst_capacity && status == VX_SUCCESS)
        {
            status |= vxTruncateArray(dst, 0);
            status |= vxAddArrayItems(dst, src_num_items, srcp, src_stride);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }

        status |= vxUnmapArrayRange(src, map_id);
    }

    return status;
} /* ownCopyArray() */

