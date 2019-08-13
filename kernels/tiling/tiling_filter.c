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

void box3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_tile_t *out = (vx_tile_t *)parameters[1];
    float32x4_t oneovernine = vdupq_n_f32(1.0f / 9.0f);
    uint8_t *src = in->base[0] + in->tile_x;
    uint8_t *dst = out->base[0] + out->tile_x;
    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    if (low_y == 0)
    {
        low_y = 1;
    }
    if (high_y == out->image.height)
    {
        high_y = high_y - 1;
    }

    for (y = low_y; y < high_y; y++)
    {
        vx_uint8* dst_u8 = (vx_uint8 *)dst + 1 + y * out->image.width;
        vx_uint8* top_src = (vx_uint8 *)src + (y - 1) * in->image.width;
        vx_uint8* mid_src = (vx_uint8 *)src + (y)* in->image.width;
        vx_uint8* bot_src = (vx_uint8 *)src + (y + 1)* in->image.width;

        for (x = 0; x < out->tile_block.width; x += 8)
        {
            const uint8x16_t top_data = vld1q_u8(top_src);
            const uint8x16_t mid_data = vld1q_u8(mid_src);
            const uint8x16_t bot_data = vld1q_u8(bot_src);

            const int16x8x2_t top_s16 =
            {
                {
                    vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(top_data))),
                    vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(top_data)))
                }
            };
            const int16x8x2_t mid_s16 =
            {
                {
                    vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(mid_data))),
                    vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(mid_data)))
                }
            };
            const int16x8x2_t bot_s16 =
            {
                {
                    vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(bot_data))),
                    vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(bot_data)))
                }
            };

            //top left
            int16x8_t vOut = top_s16.val[0];
            //top mid
            vOut = vaddq_s16(vOut, vextq_s16(top_s16.val[0], top_s16.val[1], 1));
            //top right
            vOut = vaddq_s16(vOut, vextq_s16(top_s16.val[0], top_s16.val[1], 2));
            //mid left
            vOut = vaddq_s16(vOut, mid_s16.val[0]);
            //mid mid
            vOut = vaddq_s16(vOut, vextq_s16(mid_s16.val[0], mid_s16.val[1], 1));
            //mid right
            vOut = vaddq_s16(vOut, vextq_s16(mid_s16.val[0], mid_s16.val[1], 2));
            //bot left
            vOut = vaddq_s16(vOut, bot_s16.val[0]);
            //bot mid
            vOut = vaddq_s16(vOut, vextq_s16(bot_s16.val[0], bot_s16.val[1], 1));
            //bot right
            vOut = vaddq_s16(vOut, vextq_s16(bot_s16.val[0], bot_s16.val[1], 2));

            float32x4_t outfloathigh = vcvtq_f32_s32(vmovl_s16(vget_high_s16(vOut)));
            float32x4_t outfloatlow  = vcvtq_f32_s32(vmovl_s16(vget_low_s16(vOut)));

            outfloathigh = vmulq_f32(outfloathigh, oneovernine);
            outfloatlow  = vmulq_f32(outfloatlow, oneovernine);

            vOut = vcombine_s16(vqmovn_s32(vcvtq_s32_f32(outfloatlow)),
                               vqmovn_s32(vcvtq_s32_f32(outfloathigh)));

            vst1_u8(dst_u8, vqmovun_s16(vOut));

            top_src += 8;
            mid_src += 8;
            bot_src += 8;
            dst_u8 += 8;
        }
    }
}


void box3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_tile_t *out = (vx_tile_t *)parameters[1];

    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;

    if (ty == 0 && tx == 0)
    {
        for (y = 1; y < vxTileHeight(out, 0); y++)
        {
            for (x = 1; x < vxTileWidth(out, 0); x++)
            {
                vx_int32 j, i;
                vx_uint32 sum = 0;
                vx_uint32 count = 0;
                for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)
                {
                    for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)
                    {
                        sum += vxImagePixel(vx_uint8, in, 0, x, y, i, j);
                        count++;
                    }
                }
                sum /= count;
                if (sum > 255)
                    sum = 255;
                vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
            }
        }
    }
    else
    {
        for (y = 1; y < ty; y++)
        {
            for (x = tx; x < vxTileWidth(out, 0); x++)
            {
                vx_int32 j, i;
                vx_uint32 sum = 0;
                vx_uint32 count = 0;
                for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)
                {
                    for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)
                    {

                        sum += vxImagePixel(vx_uint8, in, 0, x, y, i, j);
                        count++;
                    }
                }
                sum /= count;
                if (sum > 255)
                    sum = 255;
                vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
            }
        }

        for (y = ty; y < vxTileHeight(out, 0); y++)
        {
            for (x = 1; x < vxTileWidth(out, 0); x++)
            {
                vx_int32 j, i;
                vx_uint32 sum = 0;
                vx_uint32 count = 0;
                for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)
                {
                    for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)
                    {
                        sum += vxImagePixel(vx_uint8, in, 0, x, y, i, j);
                        count++;
                    }
                }
                sum /= count;
                if (sum > 255)
                    sum = 255;
                vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
            }
        }
    }
}
