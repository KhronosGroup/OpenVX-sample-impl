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

// nodeless version of the XXXX kernel
vx_status vxIntegralImage(vx_image src, vx_image dst)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t rect;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;

    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(src, &rect);

    status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    for (y = 0; (y < src_addr.dim_y) && (status == VX_SUCCESS); y++)
    {
        vx_uint8 *pixels = vxFormatImagePatchAddress2d(src_base, 0, y, &src_addr);
        vx_uint32 *sums = vxFormatImagePatchAddress2d(dst_base, 0, y, &dst_addr);

        if (y == 0)
        {
            sums[0] = pixels[0];
            for (x = 1; x < src_addr.dim_x; x++)
            {
                sums[x] = sums[x-1] + pixels[x];
            }
        }
        else
        {
            vx_uint32 *prev_sums = vxFormatImagePatchAddress2d(dst_base, 0, y-1, &dst_addr);
            sums[0] = prev_sums[0] + pixels[0];
            for (x = 1; x < src_addr.dim_x; x++)
            {
                sums[x] = pixels[x] + sums[x-1] + prev_sums[x] - prev_sums[x-1];
            }
        }
    }

    status |= vxUnmapImagePatch(src, src_map_id);
    status |= vxUnmapImagePatch(dst, dst_map_id);

    return status;
}

