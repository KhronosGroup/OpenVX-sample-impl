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

static vx_status vxMorphology3x3(vx_image src, vx_image dst, vx_uint8 (*op)(vx_uint8, vx_uint8), const vx_border_t *borders)
{
    vx_uint32 y, x, low_y = 0, low_x = 0, high_y, high_x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;

    vx_status status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    high_y = src_addr.dim_y;
    high_x = src_addr.dim_x;

    if (borders->mode == VX_BORDER_UNDEFINED)
    {
        /* shrink by 1 */
        vxAlterRectangle(&rect, 1, 1, -1, -1);
        low_x += 1; high_x -= 1;
        low_y += 1; high_y -= 1;
    }

    for (y = low_y; (y < high_y) && (status == VX_SUCCESS); y++)
    {
        for (x = low_x; x < high_x; x++)
        {
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            vx_uint8 pixels[9], m;
            vx_uint32 i;

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = op(m, pixels[i]);

            *dst = m;
        }
    }

    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

    return status;
}

static vx_uint8 min_op(vx_uint8 a, vx_uint8 b) {
    return a < b ? a : b;
}

static vx_uint8 max_op(vx_uint8 a, vx_uint8 b) {
    return a > b ? a : b;
}

// nodeless version of the Erode3x3 kernel
vx_status vxErode3x3(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    return vxMorphology3x3(src, dst, min_op, bordermode);
}

// nodeless version of the Dilate3x3 kernel
vx_status vxDilate3x3(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    return vxMorphology3x3(src, dst, max_op, bordermode);
}

