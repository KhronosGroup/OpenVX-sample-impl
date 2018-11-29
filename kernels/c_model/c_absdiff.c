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

// nodeless version of the AbsDiff kernel
vx_status vxAbsDiff(vx_image in1, vx_image in2, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect, r_in1, r_in2;
    vx_df_image format, dst_format;
    vx_status status = VX_SUCCESS;

    vxQueryImage(in1, VX_IMAGE_FORMAT, &format, sizeof(format));
    vxQueryImage(output, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    status  = vxGetValidRegionImage(in1, &r_in1);
    status |= vxGetValidRegionImage(in2, &r_in2);
    vxFindOverlapRectangle(&r_in1, &r_in2, &rect);
    //printf("%s Rectangle = {%u,%u x %u,%u}\n",__FUNCTION__, rect.start_x, rect.start_y, rect.end_x, rect.end_y);
    status |= vxAccessImagePatch(in1, &rect, 0, &src_addr[0], (void **)&src_base[0],VX_READ_AND_WRITE);
    status |= vxAccessImagePatch(in2, &rect, 0, &src_addr[1], (void **)&src_base[1],VX_READ_AND_WRITE);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base,VX_READ_AND_WRITE);
    height = src_addr[0].dim_y;
    width = src_addr[0].dim_x;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            if (format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *src[2] = {
                    vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                    vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]),
                };
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (*src[0] > *src[1])
                    *dst = *src[0] - *src[1];
                else
                    *dst = *src[1] - *src[0];
            }
            else if (format == VX_DF_IMAGE_S16)
            {
                vx_int16 *src[2] = {
                    vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                    vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]),
                };
                if (dst_format == VX_DF_IMAGE_S16)
                {
                    vx_int16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                    vx_uint32 val;
                    if (*src[0] > *src[1])
                        val = *src[0] - *src[1];
                    else
                        val = *src[1] - *src[0];
                    *dst = (vx_int16)((val > 32767) ? 32767 : val);
                }
                else if (dst_format == VX_DF_IMAGE_U16) {
                    vx_uint16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                    if (*src[0] > *src[1])
                        *dst = *src[0] - *src[1];
                    else
                        *dst = *src[1] - *src[0];
                }
            }
            else if (format == VX_DF_IMAGE_U16)
            {
                vx_uint16 *src[2] = {
                    vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]),
                    vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]),
                };
                vx_uint16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                if (*src[0] > *src[1])
                    *dst = *src[0] - *src[1];
                else
                    *dst = *src[1] - *src[0];
            }
        }
    }
    status |= vxCommitImagePatch(in1, NULL, 0, &src_addr[0], src_base[0]);
    status |= vxCommitImagePatch(in2, NULL, 0, &src_addr[1], src_base[1]);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);

    return status;
}

