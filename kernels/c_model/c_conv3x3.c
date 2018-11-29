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

vx_uint8 vx_clamp_u8_i32(vx_int32 value)
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

static vx_int16 vx_clamp_s16_i32(vx_int32 value)
{
    vx_int16 v = 0;
    if (value > INT16_MAX)
    {
        v = INT16_MAX;
    }
    else if (value < INT16_MIN)
    {
        v = INT16_MIN;
    }
    else
    {
        v = (vx_int16)value;
    }
    return v;
}

static vx_int32 vx_convolve8with16(void *base, vx_uint32 x, vx_uint32 y, vx_imagepatch_addressing_t *addr,
                                   vx_int16 conv[3][3], const vx_border_t *borders)
{

    vx_uint8 pixels[3][3];
    vx_int32 div, sum;
    vxReadRectangle(base, addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels[0][0]);
    div = conv[0][0] + conv[0][1] + conv[0][2] +
          conv[1][0] + conv[1][1] + conv[1][2] +
          conv[2][0] + conv[2][1] + conv[2][2];
    sum = (conv[0][0] * pixels[0][0]) + (conv[0][1] * pixels[0][1]) + (conv[0][2] * pixels[0][2]) +
          (conv[1][0] * pixels[1][0]) + (conv[1][1] * pixels[1][1]) + (conv[1][2] * pixels[1][2]) +
          (conv[2][0] * pixels[2][0]) + (conv[2][1] * pixels[2][1]) + (conv[2][2] * pixels[2][2]);
    if (div == 0)
        div = 1;
    return sum / div;
}

vx_status vxConvolution3x3(vx_image src, vx_image dst, vx_int16 conv[3][3], const vx_border_t *borders)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_enum dst_format = VX_DF_IMAGE_VIRT;
    vx_status status = VX_SUCCESS;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y;

    status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

    high_x = src_addr.dim_x;
    high_y = src_addr.dim_y;

    if (borders->mode == VX_BORDER_UNDEFINED)
    {
        ++low_x; --high_x;
        ++low_y; --high_y;
        vxAlterRectangle(&rect, 1, 1, -1, -1);
    }
    //printf("%s Rectangle = {%u,%u x %u,%u}\n",__FUNCTION__, rect.start_x, rect.start_y, rect.end_x, rect.end_y);

    for (y = low_y; y < high_y; y++)
    {
        for (x = low_x; x < high_x; x++)
        {
            vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, conv, borders);

            if (dst_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_u8_i32(value);
            }
            else
            {
                vx_int16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_s16_i32(value);
            }
        }
    }

    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
    return status;
}

