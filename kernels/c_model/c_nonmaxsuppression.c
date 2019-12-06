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

#include <stdlib.h>
#include <c_model.h>

// nodeless version of the Non Max Suppression kernel
vx_status vxNonMaxSuppression(vx_image input, vx_image mask, vx_scalar win_size, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_int32 height, width, shift_x_u1 = 0;
    vx_df_image format = 0, mask_format = 0;
    vx_uint8 mask_data = 0;
    void *src_base = NULL;
    void *mask_base = NULL;
    void *dst_base = NULL;

    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t mask_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t src_rect;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_map_id mask_map_id = 0;

    status  = vxGetValidRegionImage(input, &src_rect);
    status |= vxMapImagePatch(input,  &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY,  VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &src_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    if (mask != NULL)
    {
        status |= vxMapImagePatch(mask, &src_rect, 0, &mask_map_id, &mask_addr, (void **)&mask_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
        status |= vxQueryImage(mask, VX_IMAGE_FORMAT, &mask_format, sizeof(mask_format));
        if (mask_format == VX_DF_IMAGE_U1)
            shift_x_u1 = src_rect.start_x % 8;
    }

    status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

    width  = src_addr.dim_x;
    height = src_addr.dim_y;

    vx_int32 wsize = 0;
    status |= vxCopyScalar(win_size, &wsize, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vx_int32 border = wsize / 2;

    for (vx_int32 x = border; x < (width - border); x++)
    {
        for (vx_int32 y = border; y < (height - border); y++)
        {
            vx_uint8 *_mask;
            if (mask != NULL)
            {
                _mask = (vx_uint8 *)vxFormatImagePatchAddress2d(mask_base, x + shift_x_u1, y, &mask_addr);
                if (mask_format == VX_DF_IMAGE_U1)
                    _mask = (*_mask & (1 << ((x + shift_x_u1) % 8))) != 0 ? _mask : &mask_data;
            }
            else
            {
                _mask = &mask_data;
            }
            void *val_p = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            void *dest  = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            vx_int32 src_val = format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)val_p : *(vx_int16 *)val_p;
            if (*_mask != 0)
            {
                if (format == VX_DF_IMAGE_U8)
                    *(vx_uint8 *)dest = (vx_uint8)src_val;
                else
                    *(vx_int16 *)dest = (vx_int16)src_val;
            }
            else
            {
                vx_bool flag = 1;
                for (vx_int32 i = -border; i <= border; i++)
                {
                    for (vx_int32 j = -border; j <= border; j++)
                    {
                        void *neighbor = vxFormatImagePatchAddress2d(src_base, x + i, y + j, &src_addr);
            			if (mask != NULL)
            			{
            				_mask = (vx_uint8 *)vxFormatImagePatchAddress2d(mask_base, x + i + shift_x_u1, y + j, &mask_addr);
                            if (mask_format == VX_DF_IMAGE_U1)
                                _mask = (*_mask & (1 << ((x + i + shift_x_u1) % 8))) != 0 ? _mask : &mask_data;
            			}
            			else
            			{
            				_mask = &mask_data;
            			}
                        vx_int32 neighbor_val = format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)neighbor : *(vx_int16 *)neighbor;
                        if ( (*_mask == 0) && (((j < 0 || (j == 0 && i <= 0)) && (src_val <  neighbor_val))
            			                   ||  ((j > 0 || (j == 0 && i >  0)) && (src_val <= neighbor_val))) )
                        {
                            flag = 0;
                            break;
                        }
                    }
                    if (flag == 0)
                    {
                        break;
                    }
                }
                if (flag)
                {
                    if (format == VX_DF_IMAGE_U8)
                        *(vx_uint8 *)dest = (vx_uint8)src_val;
                    else
                        *(vx_int16 *)dest = (vx_int16)src_val;
                }
                else
                {
                    if (format == VX_DF_IMAGE_U8)
                        *(vx_uint8 *)dest = 0;
                    else
                        *(vx_int16 *)dest = INT16_MIN;
                }
            }
        }
    }
    status |= vxUnmapImagePatch(input, src_map_id);
    status |= vxUnmapImagePatch(output, dst_map_id);
    if (mask != NULL)
    {
        status |= vxUnmapImagePatch(mask, mask_map_id);
    }
    return status;
}
