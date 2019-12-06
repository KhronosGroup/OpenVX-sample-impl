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

#include <arm_neon.h>
#include <tiling.h>

void IntegralImage_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = out->tile_x + out->tile_block.width;

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint32 *dst_base = (vx_uint32 *)out->base[0] + out->tile_x;

    for (y = low_y; y < high_y; y++)
    {
        const vx_uint8 *pixels_ptr = src_base + y * in->addr->stride_y;
        vx_uint32  *sums = dst_base + y * out->addr->stride_y / 4;

        if (y == 0)
        {
            for (x = low_x; x < high_x; x += 16)
            {
                const uint8x16_t input_pixels = vld1q_u8(pixels_ptr);
                
                const uint16x8x2_t temp = 
                {
                    {
                        vmovl_u8(vget_low_u8(input_pixels)),
                        vmovl_u8(vget_high_u8(input_pixels))
                    }
                };

                uint32x4x4_t pixels = 
                {
                    {
                        vmovl_u16(vget_low_u16(temp.val[0])),
                        vmovl_u16(vget_high_u16(temp.val[0])),
                        vmovl_u16(vget_low_u16(temp.val[1])),
                        vmovl_u16(vget_high_u16(temp.val[1]))
                    }
                };

                vst1q_u32(sums, pixels.val[0]);

                vst1q_u32(sums + 4, pixels.val[1]);

                vst1q_u32(sums + 8, pixels.val[2]);

                vst1q_u32(sums + 12, pixels.val[3]);

                if (x == 0)
                {
                    sums[0] = pixels_ptr[0];

                    // Perform prefix summation
                    for (vx_int32 i = 1; i < 16; i++)
                    {
                        sums[i] += sums[i-1];
                    }
                }
                else
                {
                    // Perform prefix summation
                    for (vx_int32 i = 0; i < 16; i++)
                    {
                        sums[i] += sums[i-1];
                    }
                }

                pixels_ptr += 16;
                sums += 16;
            }
        }
        else
        {
            vx_uint32  *prev_sums_mid = dst_base + (y-1) * out->addr->stride_y / 4;    //(0,-1)
            vx_uint32  *prev_sums_left = dst_base + (y-1) * out->addr->stride_y / 4 - out->addr->stride_x / 4;  //(-1,-1)

            for (x = low_x; x < high_x; x += 16)
            {
                const uint8x16_t input_pixels = vld1q_u8(pixels_ptr);
                
                const uint16x8x2_t temp = 
                {
                    {
                        vmovl_u8(vget_low_u8(input_pixels)),
                        vmovl_u8(vget_high_u8(input_pixels))
                    }
                };

                uint32x4x4_t pixels = 
                {
                    {
                        vmovl_u16(vget_low_u16(temp.val[0])),
                        vmovl_u16(vget_high_u16(temp.val[0])),
                        vmovl_u16(vget_low_u16(temp.val[1])),
                        vmovl_u16(vget_high_u16(temp.val[1]))
                    }
                };

                // Add top mid pixel values
                pixels.val[0] = vaddq_u32(vld1q_u32(prev_sums_mid), pixels.val[0]);
                pixels.val[1] = vaddq_u32(vld1q_u32(prev_sums_mid + 4), pixels.val[1]);
                pixels.val[2] = vaddq_u32(vld1q_u32(prev_sums_mid + 8), pixels.val[2]);
                pixels.val[3] = vaddq_u32(vld1q_u32(prev_sums_mid + 12), pixels.val[3]);

                // Subtract top left diagonal values
                pixels.val[0] = vsubq_u32(pixels.val[0], vld1q_u32(prev_sums_left));
                vst1q_u32(sums, pixels.val[0]);

                pixels.val[1] = vsubq_u32(pixels.val[1], vld1q_u32(prev_sums_left + 4));
                vst1q_u32(sums + 4, pixels.val[1]);

                pixels.val[2] = vsubq_u32(pixels.val[2], vld1q_u32(prev_sums_left + 8));
                vst1q_u32(sums + 8, pixels.val[2]);

                pixels.val[3] = vsubq_u32(pixels.val[3], vld1q_u32(prev_sums_left + 12));
                vst1q_u32(sums + 12, pixels.val[3]);

                if (x == 0)
                {
                    sums[0] = prev_sums_mid[0] + pixels_ptr[0];
                    // Perform prefix summation
                    for (vx_int32 i = 1; i < 16; i++)
                    {
                        sums[i] += sums[i-1];
                    }
                }
                else
                {
                    // Perform prefix summation
                    for (vx_int32 i = 0; i < 16; i++)
                    {
                        sums[i] += sums[i-1];
                    }
                }

                pixels_ptr += 16;
                sums += 16;
                prev_sums_mid += 16;
                prev_sums_left += 16;
            }
        }
    }
}

#define INTEGRAL_IMAGE(low_y, high_y, low_x)                                                    \
    for (y = low_y; y < high_y; y++)                                                            \
    {                                                                                           \
        vx_uint8 *pixels = (vx_uint8 *)src_base + y * in->addr->stride_y;                       \
        vx_uint32 *sums = (vx_uint32 *)dst_base + y * out->addr->stride_y / 4;                  \
        if (y == 0)                                                                             \
        {                                                                                       \
            sums[0] = pixels[0];                                                                \
            for (x = low_x; x < high_x; x++)                                                    \
                sums[x] = sums[x - 1] + pixels[x];                                              \
        }                                                                                       \
        else                                                                                    \
        {                                                                                       \
            vx_uint32 *prev_sums = (vx_uint32 *)dst_base + (y - 1) * out->addr->stride_y / 4;   \
            sums[0] = prev_sums[0] + pixels[0];                                                 \
            for (x = low_x; x < high_x; x++)                                                    \
                sums[x] = pixels[x] + sums[x - 1] + prev_sums[x] - prev_sums[x - 1];            \
        }                                                                                       \
    }               

void IntegralImage_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    if (low_y == 0 && low_x == 0)
    {
        INTEGRAL_IMAGE(low_y, high_y, low_x + 1)
    }
    else
    {
        INTEGRAL_IMAGE(0, low_y, low_x)

        vx_uint8 *src_base = in->base[0];
        vx_uint8 *dst_base = out->base[0];
        INTEGRAL_IMAGE(low_y, high_y, 1)
    }
}
