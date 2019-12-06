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

#include <stdlib.h>

void box3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];
    float32x4_t oneovernine = vdupq_n_f32(1.0f / 9.0f);
    vx_uint8 *src = in->base[0] + in->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
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


void box3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];

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

static inline void sort(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t min = vmin_u8(*a, *b);
    const uint8x8_t max = vmax_u8(*a, *b);
    *a = min;
    *b = max;
}

void Median3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

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
        vx_uint8* dst = (vx_uint8 *)dst_base + 1 + y * out->addr->stride_y;
        vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * in->addr->stride_y;
        vx_uint8* mid_src = (vx_uint8 *)src_base + (y) * in->addr->stride_y;
        vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1) * in->addr->stride_y;

        for (x = 0; x < out->tile_block.width; x += 8)
        {
            const uint8x16_t top_data = vld1q_u8(top_src);
            const uint8x16_t mid_data = vld1q_u8(mid_src);
            const uint8x16_t bot_data = vld1q_u8(bot_src);

            uint8x8_t p0 = vget_low_u8(top_data);
            uint8x8_t p1 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 1);
            uint8x8_t p2 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 2);
            uint8x8_t p3 = vget_low_u8(mid_data);
            uint8x8_t p4 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 1);
            uint8x8_t p5 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 2);
            uint8x8_t p6 = vget_low_u8(bot_data);
            uint8x8_t p7 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 1);
            uint8x8_t p8 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 2);

            sort(&p1, &p2);
            sort(&p4, &p5);
            sort(&p7, &p8);

            sort(&p0, &p1);
            sort(&p3, &p4);
            sort(&p6, &p7);

            sort(&p1, &p2);
            sort(&p4, &p5);
            sort(&p7, &p8);

            sort(&p0, &p3);
            sort(&p5, &p8);
            sort(&p4, &p7);

            sort(&p3, &p6);
            sort(&p1, &p4);
            sort(&p2, &p5);

            sort(&p4, &p7);
            sort(&p4, &p2);
            sort(&p6, &p4);

            sort(&p4, &p2);

            vst1_u8(dst, p4);

            top_src+=8;
            mid_src+=8;
            bot_src+=8;
            dst += 8;
        }
    }
}


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


#define Median3x3(low_y, high_y, low_x, high_x)                                        \
    for (y = low_y; y < high_y; y++)                                                   \
    {                                                                                  \
        for (x = low_x; x < high_x; x++)                                               \
        {                                                                              \
            vx_int32 j, i;                                                             \
            vx_uint8 values[9];                                                        \
            vx_uint32 count = 0;                                                       \
            for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)        \
            {                                                                          \
                for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)    \
                {                                                                      \
                   values[count++] = vxImagePixel(vx_uint8, in, 0, x, y, i, j);        \
                }                                                                      \
            }                                                                          \
            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);          \
            vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = values[4];                    \
        }                                                                              \
    }


static void vxReadRectangle_U1(const void *base,
                           const vx_imagepatch_addressing_t *addr,
                           vx_uint32 center_x,
                           vx_uint32 center_y,
                           vx_uint32 radius_x,
                           vx_uint32 radius_y,
                           void *destination,
                           vx_uint32 border_x_start)
{
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    vx_uint16 stride_x_bits = addr->stride_x_bits;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 dest_index = 0;
    // kx, ky - kernel x and y

    for (ky = -(int32_t)radius_y; ky <= (int32_t)radius_y; ++ky)
    {
        vx_int32 y = (vx_int32)(center_y + ky);
        y = y < 0 ? 0 : y >= height ? height - 1 : y;

        for (kx = -(int32_t)radius_x; kx <= (int32_t)radius_x; ++kx, ++dest_index)
        {
            vx_int32 x = (int32_t)(center_x + kx);
            x = x < (int32_t)border_x_start ? (int32_t)border_x_start : x >= width ? width - 1 : x;

            ((vx_uint8*)destination)[dest_index] =
                ( *(vx_uint8*)(ptr + y*stride_y + vxDivFloor(x*stride_x_bits, 8)) & (1 << (x % 8)) ) >> (x % 8);

        }
    }
}

void Median3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    if (in->is_U1 == 0)
    {
        if (low_y == 0 && low_x == 0)
        {
            Median3x3(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
        }
        else
        {
            Median3x3(1, low_y, low_x, high_x - 1)
            Median3x3(low_y, high_y, 1, high_x - 1)
        }
    }
    else
    {
        void *src_base = in->base[0];                                                           
        void *dst_base = out->base[0];    
        vx_uint32 shift_x_u1 = in->rect.start_x % 8;                                                         
        high_x = high_x - shift_x_u1;                                                                  
                                         
        ++low_x; --high_x;
        ++low_y; --high_y;                                                          
        for (y = low_y; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x++)
            {
                vx_uint32 xShftd = x + shift_x_u1;      // Bit-shift for U1 valid region start
                vx_uint8 *dst_ptr = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xShftd, y, out->addr);
                vx_uint8 values[9];

                vxReadRectangle_U1(src_base, in->addr, xShftd, y, 1, 1, values, shift_x_u1);

                qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
                /* pick the middle value */
                *dst_ptr = (*dst_ptr & ~(1 << (xShftd % 8))) | (values[4] << (xShftd % 8));
            }
        }
    }
}


void Gaussian3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    int16x8_t two  = vdupq_n_s16(2);
    int16x8_t four = vdupq_n_s16(4);

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
        vx_uint8* dst = (vx_uint8 *)dst_base + 1 + y * out->addr->stride_y;
        vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * in->addr->stride_y;
        vx_uint8* mid_src = (vx_uint8 *)src_base + (y) * in->addr->stride_y;
        vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1) * in->addr->stride_y;

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
            int16x8_t out = top_s16.val[0];
            //top mid
            out = vmlaq_s16(out, vextq_s16(top_s16.val[0], top_s16.val[1], 1), two);
            //top right
            out = vaddq_s16(out, vextq_s16(top_s16.val[0], top_s16.val[1], 2));
            //mid left
            out = vmlaq_s16(out, mid_s16.val[0], two);
            //mid mid
            out = vmlaq_s16(out, vextq_s16(mid_s16.val[0], mid_s16.val[1], 1), four);
            //mid right
            out = vmlaq_s16(out, vextq_s16(mid_s16.val[0], mid_s16.val[1], 2), two);
            //bot left
            out = vaddq_s16(out, bot_s16.val[0]);
            //bot mid
            out = vmlaq_s16(out, vextq_s16(bot_s16.val[0], bot_s16.val[1], 1), two);
            //bot right
            out = vaddq_s16(out, vextq_s16(bot_s16.val[0], bot_s16.val[1], 2));

            vst1_u8(dst, vqshrun_n_s16(out, 4));

            top_src+=8;
            mid_src+=8;
            bot_src+=8;
            dst += 8;
        }
    }
}

#define Gaussian3x3(low_y, high_y, low_x, high_x)                          \
    for (y = low_y; y < high_y; y++)                                       \
    {                                                                      \
        for (x = low_x; x < high_x; x++)                                   \
        {                                                                  \
            vx_uint32 sum = 0;                                             \
                                                                           \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, -1, -1);            \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, 0, -1) << 1;        \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, +1, -1);            \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, -1, 0) << 1;        \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, 0, 0) << 2;         \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, +1, 0) << 1;        \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, -1, +1);            \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, 0, +1) << 1;        \
            sum += vxImagePixel(vx_uint8, in, 0, x, y, +1, +1);            \
            sum >>= 4;                                                     \
            if (sum > 255)                                                 \
                sum = 255;                                                 \
            vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;    \
        }                                                                  \
    }


void Gaussian3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    if (low_y == 0 && low_x == 0)
    {
        Gaussian3x3(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
    }
    else
    {
        Gaussian3x3(1, low_y, low_x, high_x - 1)
        Gaussian3x3(low_y, high_y, 1, high_x - 1)
    }
}
