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

#include "extras_k.h"

static vx_int16 laplacian[3][3] =
{
    { 1, 1, 1 },
    { 1,-8, 1 },
    { 1, 1, 1 },
};

static vx_uint8 vx_clamp8_i32(vx_int32 value)
{
    vx_uint8 v = 0;
    if (value > 255)
    {
        v = 255;
    }
    else if (value < 0)
    {
        v = 0;
    }
    else
    {
        v = (vx_uint8)value;
    }
    return v;
}

vx_int32 own_convolve8with16(void* base, vx_uint32 x, vx_uint32 y, vx_imagepatch_addressing_t* addr, vx_int16 conv[3][3])
{
    vx_int32 stride_y = (addr->stride_y * addr->scale_y)/VX_SCALE_UNITY;
    vx_int32 stride_x = (addr->stride_x * addr->scale_x)/VX_SCALE_UNITY;

    vx_uint8* ptr = (vx_uint8*)base;
    vx_uint32 i = (y * stride_y) + (x * stride_x);

    vx_uint32 indexes[3][3] =
    {
        { i - stride_y - stride_x, i - stride_y, i - stride_y + stride_x },
        { i - stride_x,            i,            i + stride_x },
        { i + stride_y - stride_x, i + stride_y, i + stride_y + stride_x },
    };

    vx_uint8 pixels[3][3] =
    {
        { ptr[indexes[0][0]], ptr[indexes[0][1]], ptr[indexes[0][2]] },
        { ptr[indexes[1][0]], ptr[indexes[1][1]], ptr[indexes[1][2]] },
        { ptr[indexes[2][0]], ptr[indexes[2][1]], ptr[indexes[2][2]] },
    };

    vx_int32 div = conv[0][0] + conv[0][1] + conv[0][2] +
                   conv[1][0] + conv[1][1] + conv[1][2] +
                   conv[2][0] + conv[2][1] + conv[2][2];

    vx_int32 sum = (conv[0][0] * pixels[0][0]) + (conv[0][1] * pixels[0][1]) + (conv[0][2] * pixels[0][2]) +
                   (conv[1][0] * pixels[1][0]) + (conv[1][1] * pixels[1][1]) + (conv[1][2] * pixels[1][2]) +
                   (conv[2][0] * pixels[2][0]) + (conv[2][1] * pixels[2][1]) + (conv[2][2] * pixels[2][2]);

    if (div == 0)
        div = 1;

    return sum / div;
} /* own_convolve8with16() */

// nodeless version of the Laplacian3x3 kernel
vx_status ownLaplacian3x3(vx_image src, vx_image dst, vx_border_t* bordermode)
{
    vx_uint32 x;
    vx_uint32 y;
    void* src_base = NULL;
    void* dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status |= vxGetValidRegionImage(src, &rect);

    status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

    /*! \todo Implement other border modes */
    if (bordermode->mode == VX_BORDER_UNDEFINED)
    {
        /* shrink the image by 1 */
        vxAlterRectangle(&rect, 1, 1, -1, -1);

        for (y = 1; y < (src_addr.dim_y - 1); y++)
        {
            for (x = 1; x < (src_addr.dim_x - 1); x++)
            {
                vx_int32 value = own_convolve8with16(src_base, x, y, &src_addr, laplacian);
                vx_uint8* dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp8_i32(value);
            }
        }
    }
    else
    {
        status = VX_ERROR_NOT_IMPLEMENTED;
    }

    status |= vxUnmapImagePatch(src, src_map_id);
    status |= vxUnmapImagePatch(dst, dst_map_id);

    return status;
} /* ownLaplacian3x3() */

