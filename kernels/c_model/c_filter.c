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


// helpers
static int vx_uint8_compare(const void *p1, const void *p2)
{
    vx_uint8 a = *(vx_uint8 *)p1;
    vx_uint8 b = *(vx_uint8 *)p2;
    if (a > b)
        return 1;
    else if (a == b)
        return 0;
    else
        return -1;
}


// nodeless version of the Median3x3 kernel
vx_status vxMedian3x3(vx_image src, vx_image dst, vx_border_t *borders)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_df_image format = 0;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y, shift_x_u1;

    vx_status status = vxGetValidRegionImage(src, &rect);
    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    shift_x_u1 = (format == VX_DF_IMAGE_U1) ? rect.start_x % 8 : 0;
    high_x = src_addr.dim_x - shift_x_u1;   // U1 addressing rounds down imagepatch start_x to nearest byte boundary
    high_y = src_addr.dim_y;

    if (borders->mode == VX_BORDER_UNDEFINED)
    {
        ++low_x; --high_x;
        ++low_y; --high_y;
        vxAlterRectangle(&rect, 1, 1, -1, -1);
    }

    for (y = low_y; (y < high_y) && (status == VX_SUCCESS); y++)
    {
        for (x = low_x; x < high_x; x++)
        {
            vx_uint32 xShftd = x + shift_x_u1;      // Bit-shift for U1 valid region start
            vx_uint8 *dst_ptr = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xShftd, y, &dst_addr);
            vx_uint8 values[9];

            vxReadRectangle(src_base, &src_addr, borders, format, xShftd, y, 1, 1, values, shift_x_u1);

            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
            /* pick the middle value */
            if (format == VX_DF_IMAGE_U1)
                *dst_ptr = (*dst_ptr & ~(1 << (xShftd % 8))) | (values[4] << (xShftd % 8));
            else
                *dst_ptr = values[4];
        }
    }

    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

    return status;
}


// nodeless version of the Box3x3 kernel
static vx_int16 box[3][3] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1},
};

vx_status vxBox3x3(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    return vxConvolution3x3(src, dst, box, bordermode);
}


// nodeless version of the Gaussian3x3 kernel
static vx_int16 gaussian[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1},
};

vx_status vxGaussian3x3(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    return vxConvolution3x3(src, dst, gaussian, bordermode);
}

