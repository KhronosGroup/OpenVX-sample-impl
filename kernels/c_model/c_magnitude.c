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

// nodeless version of the Magnitude kernel
vx_status vxMagnitude(vx_image grad_x, vx_image grad_y, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_uint32 y, x;
    vx_df_image format = 0;
    vx_uint8 *dst_base   = NULL;
    vx_int16 *src_base_x = NULL;
    vx_int16 *src_base_y = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr_x, src_addr_y;
    vx_rectangle_t rect;
    vx_uint32 value;

    if (grad_x == 0 || grad_y == 0)
        return VX_ERROR_INVALID_PARAMETERS;

    vxQueryImage(output, VX_IMAGE_FORMAT, &format, sizeof(format));
    status = vxGetValidRegionImage(grad_x, &rect);
    status |= vxAccessImagePatch(grad_x, &rect, 0, &src_addr_x, (void **)&src_base_x, VX_READ_ONLY);
    status |= vxAccessImagePatch(grad_y, &rect, 0, &src_addr_y, (void **)&src_base_y, VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
    for (y = 0; y < src_addr_x.dim_y; y++)
    {
        for (x = 0; x < src_addr_x.dim_x; x++)
        {
            vx_int16 *in_x = vxFormatImagePatchAddress2d(src_base_x, x, y, &src_addr_x);
            vx_int16 *in_y = vxFormatImagePatchAddress2d(src_base_y, x, y, &src_addr_y);
            if (format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_int32 grad[2] = {in_x[0]*in_x[0], in_y[0]*in_y[0]};
                vx_float64 sum = grad[0] + grad[1];
                value = ((vx_int32)sqrt(sum))/4;
                *dst = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
            }
            else if (format == VX_DF_IMAGE_S16)
            {
                vx_uint16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_float64 grad[2] = {(vx_float64)in_x[0]*in_x[0], (vx_float64)in_y[0]*in_y[0]};
                vx_float64 sum = grad[0] + grad[1];
                value = (vx_int32)(sqrt(sum) + 0.5);
                *dst = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
            }
        }
    }
    status |= vxCommitImagePatch(grad_x, NULL, 0, &src_addr_x, src_base_x);
    status |= vxCommitImagePatch(grad_y, NULL, 0, &src_addr_y, src_base_y);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);
    return status;
}

