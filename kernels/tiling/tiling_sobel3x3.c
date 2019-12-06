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

#define SOBEL3x3_VALUE                                                 \
    const uint8x16_t top_data = vld1q_u8(top_src);                     \
    const uint8x16_t mid_data = vld1q_u8(mid_src);                     \
    const uint8x16_t bot_data = vld1q_u8(bot_src);                     \
                                                                       \
    const int16x8x2_t top_s16 =                                        \
    {                                                                  \
        {                                                              \
            vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(top_data))),    \
            vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(top_data)))    \
        }                                                              \
    };                                                                 \
                                                                       \
    const int16x8x2_t mid_s16 =                                        \
    {                                                                  \
        {                                                              \
            vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(mid_data))),    \
            vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(mid_data)))    \
        }                                                              \
    };                                                                 \
    const int16x8x2_t bot_s16 =                                        \
    {                                                                  \
        {                                                              \
            vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(bot_data))),    \
            vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(bot_data)))    \
        }                                                              \
    };

void Sobel3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    int16x8_t two      = vdupq_n_s16(2);
    int16x8_t minustwo = vdupq_n_s16(-2);

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *grad_x = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *grad_y = (vx_tile_ex_t *)parameters[2];

    vx_uint8 *src_base = in->base[0] + in->tile_x;

    if (grad_x)
    {
        vx_int16 *grad_x_base = (vx_int16 *)grad_x->base[0] + grad_x->tile_x;

        vx_uint32 low_y = grad_x->tile_y;
        vx_uint32 high_y = grad_x->tile_y + grad_x->tile_block.height;

        if (low_y == 0)
        {
            low_y = 1;
        }
        if (high_y == grad_x->image.height)
        {
            high_y = high_y - 1;
        }

        for (y = low_y; y < high_y; y++)
        {
            vx_int16* dstp = (vx_int16 *)grad_x_base + 1 + y * grad_x->addr->stride_y / 2;
            vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * in->addr->stride_y;
            vx_uint8* mid_src = (vx_uint8 *)src_base + (y) * in->addr->stride_y;
            vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1) * in->addr->stride_y;

            for (x = 0; x < grad_x->tile_block.width; x += 8)
            {
                SOBEL3x3_VALUE           
                //top left
                int16x8_t out_x = vnegq_s16(top_s16.val[0]);
                //top right
                out_x = vaddq_s16(out_x, vextq_s16(top_s16.val[0], top_s16.val[1], 2));
                //mid left
                out_x = vmlaq_s16(out_x, mid_s16.val[0], minustwo);
                //mid right
                out_x = vmlaq_s16(out_x, vextq_s16(mid_s16.val[0], mid_s16.val[1], 2), two);
                //bot left
                out_x = vsubq_s16(out_x, bot_s16.val[0]);
                //bot right
                out_x = vaddq_s16(out_x, vextq_s16(bot_s16.val[0], bot_s16.val[1], 2));

                vst1q_s16(dstp, out_x);

                top_src+=8;
                mid_src+=8;
                bot_src+=8;
                dstp += 8;
            }
        }
    }
    if (grad_y)
    {
        vx_int16 *grad_y_base = (vx_int16 *)grad_y->base[0] + grad_y->tile_x;

        vx_uint32 low_y = grad_y->tile_y;
        vx_uint32 high_y = grad_y->tile_y + grad_y->tile_block.height;

        if (low_y == 0)
        {
            low_y = 1;
        }
        if (high_y == grad_y->image.height)
        {
            high_y = high_y - 1;
        }

        for (y = low_y; y < high_y; y++)
        {
            vx_int16* dstp = (vx_int16 *)grad_y_base + 1 + y * grad_y->addr->stride_y / 2;
            vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * in->addr->stride_y;
            vx_uint8* mid_src = (vx_uint8 *)src_base + (y) * in->addr->stride_y;
            vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1) * in->addr->stride_y;

            for (x = 0; x < grad_y->tile_block.width; x += 8)
            {
                SOBEL3x3_VALUE           
                //top left
                int16x8_t out_y = vnegq_s16(top_s16.val[0]);
                //top mid
                out_y = vmlaq_s16(out_y, vextq_s16(top_s16.val[0], top_s16.val[1], 1), minustwo);
                //top right
                out_y = vsubq_s16(out_y, vextq_s16(top_s16.val[0], top_s16.val[1], 2));
                //bot left
                out_y = vaddq_s16(out_y, bot_s16.val[0]);
                //bot mid
                out_y = vmlaq_s16(out_y, vextq_s16(bot_s16.val[0], bot_s16.val[1], 1), two);
                //bot right
                out_y = vaddq_s16(out_y, vextq_s16(bot_s16.val[0], bot_s16.val[1], 2));

                vst1q_s16(dstp, out_y);

                top_src+=8;
                mid_src+=8;
                bot_src+=8;
                dstp += 8;
            }
        }
    }
}


#define SOBEL3x3_X(low_y, high_y, low_x, high_x)                                    \
        for (y = low_y; y < high_y; y++)                                            \
        {                                                                           \
            for (x = low_x; x < high_x; x++)                                        \
            {                                                                       \
                vx_int32 value = 0;                                                 \
                                                                                    \
                value -= vxImagePixel(vx_uint8, in, 0, x, y, -1, -1);               \
                value += vxImagePixel(vx_uint8, in, 0, x, y, +1, -1);               \
                value -= vxImagePixel(vx_uint8, in, 0, x, y, -1, 0) << 1;           \
                value += vxImagePixel(vx_uint8, in, 0, x, y, +1, 0) << 1;           \
                value -= vxImagePixel(vx_uint8, in, 0, x, y, -1, +1);               \
                value += vxImagePixel(vx_uint8, in, 0, x, y, +1, +1);               \
                                                                                    \
                vxImagePixel(vx_int16, grad_x, 0, x, y, 0, 0) = (vx_int16)value;    \
            }                                                                       \
        }                                                                           

#define SOBEL3x3_Y(low_y, high_y, low_x, high_x)                                    \
        for (y = low_y; y < high_y; y++)                                            \
        {                                                                           \
            for (x = low_x; x < high_x; x++)                                        \
            {                                                                       \
                vx_int32 value = 0;                                                 \
                                                                                    \
                value -= vxImagePixel(vx_uint8, in, 0, x, y, -1, -1);               \
                value -= vxImagePixel(vx_uint8, in, 0, x, y, 0, -1) << 1;           \
                value -= vxImagePixel(vx_uint8, in, 0, x, y, +1, -1);               \
                value += vxImagePixel(vx_uint8, in, 0, x, y, -1, +1);               \
                value += vxImagePixel(vx_uint8, in, 0, x, y, 0, +1) << 1;           \
                value += vxImagePixel(vx_uint8, in, 0, x, y, +1, +1);               \
                                                                                    \
                vxImagePixel(vx_int16, grad_y, 0, x, y, 0, 0) = (vx_int16)value;    \
            }                                                                       \
        }                                                                           


void Sobel3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *grad_x = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *grad_y = (vx_tile_ex_t *)parameters[2];

    if (grad_x)
    {
        vx_uint32 low_y = grad_x->tile_y;                                           
        vx_uint32 high_y = vxTileHeight(grad_x, 0);                                 
        vx_uint32 low_x = grad_x->tile_x;                                           
        vx_uint32 high_x = vxTileWidth(grad_x, 0);                                  
        if (low_y == 0 && low_x == 0)
        {
            SOBEL3x3_X(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
        }
        else
        {
            SOBEL3x3_X(1, low_y, low_x, high_x - 1)
            SOBEL3x3_X(low_y, high_y, 1, high_x - 1)
        }
    }
    if (grad_y)
    {
        vx_uint32 low_y = grad_y->tile_y;
        vx_uint32 high_y = vxTileHeight(grad_y, 0);
        vx_uint32 low_x = grad_y->tile_x;
        vx_uint32 high_x = vxTileWidth(grad_y, 0);
        if (low_y == 0 && low_x == 0)
        {
            SOBEL3x3_Y(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
        }
        else
        {
            SOBEL3x3_Y(1, low_y, low_x, high_x - 1)
            SOBEL3x3_Y(low_y, high_y, 1, high_x - 1)
        }
    }
}
