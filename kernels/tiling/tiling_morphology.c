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

static inline void opt_max(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t max = vmax_u8(*a, *b);
    *a = max;
}
static inline void opt_min(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t min = vmin_u8(*a, *b);
    *a = min;
}

void Erode3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
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

            opt_min(&p0, &p1);
            opt_min(&p0, &p2);
            opt_min(&p0, &p3);
            opt_min(&p0, &p4);
            opt_min(&p0, &p5);
            opt_min(&p0, &p6);
            opt_min(&p0, &p7);
            opt_min(&p0, &p8);

            vst1_u8(dst, p0);       
            
            top_src+=8;
            mid_src+=8;
            bot_src+=8;
            dst += 8;
        }
    }
}

#define Erode3x3(low_y, high_y, low_x, high_x)                                         \
    for (y = low_y; y < high_y; y++)                                                   \
    {                                                                                  \
        for (x = low_x; x < high_x; x++)                                               \
        {                                                                              \
            vx_int32 j, i;                                                             \
            vx_uint8 min_pixel = vxImagePixel(vx_uint8, in, 0, x, y, -1, -1);          \
            for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)        \
            {                                                                          \
                for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)    \
                {                                                                      \
                    if (min_pixel < vxImagePixel(vx_uint8, in, 0, x, y, i, j))         \
                        min_pixel = min_pixel;                                         \
                    else                                                               \
                        min_pixel = vxImagePixel(vx_uint8, in, 0, x, y, i, j);         \
                }                                                                      \
            }                                                                          \
            vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = min_pixel;                    \
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

static vx_uint8 min_op(vx_uint8 a, vx_uint8 b) {
    return a < b ? a : b;
}

static vx_uint8 max_op(vx_uint8 a, vx_uint8 b) {
    return a > b ? a : b;
}

#define vxMorphology3x3(op)                                                                                \
    void *src_base = in->base[0];                                                                          \
    void *dst_base = out->base[0];                                                                         \
    vx_uint32 shift_x_u1 = in->rect.start_x % 8;                                                           \
    high_x = high_x - shift_x_u1;                                                                          \
                                                                                                           \
    ++low_x; --high_x;                                                                                     \
    ++low_y; --high_y;                                                                                     \
    for (y = low_y; y < high_y; y++)                                                                       \
    {                                                                                                      \
        for (x = low_x; x < high_x; x++)                                                                   \
        {                                                                                                  \
            vx_uint32 xShftd = x + shift_x_u1;                                                             \
            vx_uint8 *dst_ptr = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xShftd, y, out->addr);    \
            vx_uint8 pixels[9], m;                                                                         \
                                                                                                           \
            vxReadRectangle_U1(src_base, in->addr, xShftd, y, 1, 1, &pixels, shift_x_u1);                  \
                                                                                                           \
            m = pixels[0];                                                                                 \
            for (vx_uint32 i = 1; i < dimof(pixels); i++)                                                  \
                m = op(m, pixels[i]);                                                                      \
                                                                                                           \
            *dst_ptr = (*dst_ptr & ~(1 << (xShftd % 8))) | (m << (xShftd % 8));                            \
        }                                                                                                  \
    }                                                                                                      


void Erode3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
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
            Erode3x3(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
        }
        else
        {
            Erode3x3(1, low_y, low_x, high_x - 1)
            Erode3x3(low_y, high_y, 1, high_x - 1)
        }
    }
    else
    {
        vxMorphology3x3(min_op)
    }
}


void Dilate3x3_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
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

            opt_max(&p0, &p1);
            opt_max(&p0, &p2);
            opt_max(&p0, &p3);
            opt_max(&p0, &p4);
            opt_max(&p0, &p5);
            opt_max(&p0, &p6);
            opt_max(&p0, &p7);
            opt_max(&p0, &p8);

            vst1_u8(dst, p0);

            top_src+=8;
            mid_src+=8;
            bot_src+=8;
            dst += 8;
        }
    }
}


#define Dilate3x3(low_y, high_y, low_x, high_x)                                        \
    for (y = low_y; y < high_y; y++)                                                   \
    {                                                                                  \
        for (x = low_x; x < high_x; x++)                                               \
        {                                                                              \
            vx_int32 j, i;                                                             \
            vx_uint8 max_pixel = vxImagePixel(vx_uint8, in, 0, x, y, -1, -1);          \
            for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)        \
            {                                                                          \
                for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)    \
                {                                                                      \
                    if (max_pixel > vxImagePixel(vx_uint8, in, 0, x, y, i, j))         \
                        max_pixel = max_pixel;                                         \
                    else                                                               \
                        max_pixel = vxImagePixel(vx_uint8, in, 0, x, y, i, j);         \
                }                                                                      \
            }                                                                          \
            vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = max_pixel;                    \
        }                                                                              \
    }


void Dilate3x3_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
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
            Dilate3x3(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
        }
        else
        {
            Dilate3x3(1, low_y, low_x, high_x - 1)
            Dilate3x3(low_y, high_y, 1, high_x - 1)
        }
    }
    else
    {
        vxMorphology3x3(max_op)
    }
}
