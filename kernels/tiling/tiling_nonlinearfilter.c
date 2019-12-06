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

struct src_ptr
{
    vx_uint8* top2_src;
    vx_uint8* top_src;
    vx_uint8* mid_src;
    vx_uint8* bot_src;
    vx_uint8* bot2_src;
};


static void sort(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t min = vmin_u8(*a, *b);
    const uint8x8_t max = vmax_u8(*a, *b);
    *a                   = min;
    *b                   = max;
}

static void sort_min(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t min = vmin_u8(*a, *b);
    *a                   = min;
}

static void sort_max(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t max = vmax_u8(*a, *b);
    *a                   = max;
}

// Calculations that do not affect the median were removed.
static void sort5_mid(uint8x8_t *p0, uint8x8_t *p1, uint8x8_t *p2, uint8x8_t *p3, uint8x8_t *p4)
{
    sort(p0, p1);
    sort(p2, p3);
    sort(p0, p2);
    sort(p1, p3);
    sort(p1, p2);
    sort(p0, p4);
    sort(p1, p4);
    sort(p2, p4);
}

static void sort5_min(uint8x8_t *p0, uint8x8_t *p1, uint8x8_t *p2, uint8x8_t *p3, uint8x8_t *p4)
{
    sort_min(p0, p1);
    sort_min(p0, p2);
    sort_min(p0, p3);
    sort_min(p0, p4);
}

static void sort5_max(uint8x8_t *p0, uint8x8_t *p1, uint8x8_t *p2, uint8x8_t *p3, uint8x8_t *p4)
{
    sort_max(p0, p1);
    sort_max(p0, p2);
    sort_max(p0, p3);
    sort_max(p0, p4);
}

static void sort9_mid(uint8x8_t *p0, uint8x8_t *p1, uint8x8_t *p2,
                      uint8x8_t *p3, uint8x8_t *p4, uint8x8_t *p5,
                      uint8x8_t *p6, uint8x8_t *p7, uint8x8_t *p8)
{
    sort(p1, p2);
    sort(p4, p5);
    sort(p7, p8);
    sort(p0, p1);
    sort(p3, p4);
    sort(p6, p7);
    sort(p1, p2);
    sort(p4, p5);
    sort(p7, p8);
    sort(p0, p3);
    sort(p5, p8);
    sort(p4, p7);
    sort(p3, p6);
    sort(p1, p4);
    sort(p2, p5);
    sort(p4, p7);
    sort(p4, p2);
    sort(p6, p4);
    sort(p4, p2);
}

static void sort9_min(uint8x8_t *p0, uint8x8_t *p1, uint8x8_t *p2,
                      uint8x8_t *p3, uint8x8_t *p4, uint8x8_t *p5,
                      uint8x8_t *p6, uint8x8_t *p7, uint8x8_t *p8)
{
    sort_min(p0, p1);
    sort_min(p0, p2);
    sort_min(p0, p3);
    sort_min(p0, p4);
    sort_min(p0, p5);
    sort_min(p0, p6);
    sort_min(p0, p7);
    sort_min(p0, p8);
}

static void sort9_max(uint8x8_t *p0, uint8x8_t *p1, uint8x8_t *p2,
                      uint8x8_t *p3, uint8x8_t *p4, uint8x8_t *p5,
                      uint8x8_t *p6, uint8x8_t *p7, uint8x8_t *p8)
{
    sort_max(p0, p1);
    sort_max(p0, p2);
    sort_max(p0, p3);
    sort_max(p0, p4);
    sort_max(p0, p5);
    sort_max(p0, p6);
    sort_max(p0, p7);
    sort_max(p0, p8);
}

static void sort21_mid(uint8x8_t p[21])
{
    sort(&p[0], &p[1]);
    sort(&p[2], &p[3]);
    sort(&p[4], &p[5]);
    sort(&p[6], &p[7]);
    sort(&p[8], &p[9]);
    sort(&p[10], &p[11]);
    sort(&p[12], &p[13]);
    sort(&p[14], &p[15]);
    sort(&p[16], &p[17]);
    sort(&p[18], &p[19]);
    sort(&p[0], &p[2]);
    sort(&p[1], &p[3]);
    sort(&p[4], &p[6]);
    sort(&p[5], &p[7]);
    sort(&p[8], &p[10]);
    sort(&p[9], &p[11]);
    sort(&p[12], &p[14]);
    sort(&p[13], &p[15]);
    sort(&p[16], &p[18]);
    sort(&p[17], &p[19]);
    sort(&p[1], &p[2]);
    sort(&p[5], &p[6]);
    sort(&p[0], &p[4]);
    sort(&p[3], &p[7]);
    sort(&p[9], &p[10]);
    sort(&p[13], &p[14]);
    sort(&p[8], &p[12]);
    sort(&p[11], &p[15]);
    sort(&p[17], &p[18]);
    sort(&p[16], &p[20]);
    sort(&p[1], &p[5]);
    sort(&p[2], &p[6]);
    sort(&p[9], &p[13]);
    sort(&p[10], &p[14]);
    sort(&p[0], &p[8]);
    sort(&p[7], &p[15]);
    sort(&p[17], &p[20]);
    sort(&p[1], &p[4]);
    sort(&p[3], &p[6]);
    sort(&p[9], &p[12]);
    sort(&p[11], &p[14]);
    sort(&p[18], &p[20]);
    sort(&p[0], &p[16]);
    sort(&p[2], &p[4]);
    sort(&p[3], &p[5]);
    sort(&p[10], &p[12]);
    sort(&p[11], &p[13]);
    sort(&p[1], &p[9]);
    sort(&p[6], &p[14]);
    sort(&p[19], &p[20]);
    sort(&p[3], &p[4]);
    sort(&p[11], &p[12]);
    sort(&p[1], &p[8]);
    sort(&p[2], &p[10]);
    sort(&p[5], &p[13]);
    sort(&p[7], &p[14]);
    sort(&p[3], &p[11]);
    sort(&p[2], &p[8]);
    sort(&p[4], &p[12]);
    sort(&p[7], &p[13]);
    sort(&p[1], &p[17]);
    sort(&p[3], &p[10]);
    sort(&p[5], &p[12]);
    sort(&p[1], &p[16]);
    sort(&p[2], &p[18]);
    sort(&p[3], &p[9]);
    sort(&p[6], &p[12]);
    sort(&p[2], &p[16]);
    sort(&p[3], &p[8]);
    sort(&p[7], &p[12]);
    sort(&p[5], &p[9]);
    sort(&p[6], &p[10]);
    sort(&p[4], &p[8]);
    sort(&p[7], &p[11]);
    sort(&p[3], &p[19]);
    sort(&p[5], &p[8]);
    sort(&p[7], &p[10]);
    sort(&p[3], &p[18]);
    sort(&p[4], &p[20]);
    sort(&p[6], &p[8]);
    sort(&p[7], &p[9]);
    sort(&p[3], &p[17]);
    sort(&p[5], &p[20]);
    sort(&p[7], &p[8]);
    sort(&p[3], &p[16]);
    sort(&p[6], &p[20]);
    sort(&p[5], &p[17]);
    sort(&p[7], &p[20]);
    sort(&p[4], &p[16]);
    sort(&p[6], &p[18]);
    sort(&p[5], &p[16]);
    sort(&p[7], &p[19]);
    sort(&p[7], &p[18]);
    sort(&p[6], &p[16]);
    sort(&p[7], &p[17]);
    sort(&p[10], &p[18]);
    sort(&p[7], &p[16]);
    sort(&p[9], &p[17]);
    sort(&p[8], &p[16]);
    sort(&p[9], &p[16]);
    sort(&p[10], &p[16]);
}

static void sort21_min(uint8x8_t p[21])
{
    sort_min(&p[0], &p[1]);
    sort_min(&p[0], &p[2]);
    sort_min(&p[0], &p[3]);
    sort_min(&p[0], &p[4]);
    sort_min(&p[0], &p[5]);
    sort_min(&p[0], &p[6]);
    sort_min(&p[0], &p[7]);
    sort_min(&p[0], &p[8]);
    sort_min(&p[0], &p[9]);
    sort_min(&p[0], &p[10]);
    sort_min(&p[0], &p[11]);
    sort_min(&p[0], &p[12]);
    sort_min(&p[0], &p[13]);
    sort_min(&p[0], &p[14]);
    sort_min(&p[0], &p[15]);
    sort_min(&p[0], &p[16]);
    sort_min(&p[0], &p[17]);
    sort_min(&p[0], &p[18]);
    sort_min(&p[0], &p[19]);
    sort_min(&p[0], &p[20]);
}


static void sort21_max(uint8x8_t p[21])
{
    sort_max(&p[0], &p[1]);
    sort_max(&p[0], &p[2]);
    sort_max(&p[0], &p[3]);
    sort_max(&p[0], &p[4]);
    sort_max(&p[0], &p[5]);
    sort_max(&p[0], &p[6]);
    sort_max(&p[0], &p[7]);
    sort_max(&p[0], &p[8]);
    sort_max(&p[0], &p[9]);
    sort_max(&p[0], &p[10]);
    sort_max(&p[0], &p[11]);
    sort_max(&p[0], &p[12]);
    sort_max(&p[0], &p[13]);
    sort_max(&p[0], &p[14]);
    sort_max(&p[0], &p[15]);
    sort_max(&p[0], &p[16]);
    sort_max(&p[0], &p[17]);
    sort_max(&p[0], &p[18]);
    sort_max(&p[0], &p[19]);
    sort_max(&p[0], &p[20]);
}

static void sort25_mid(uint8x8_t p[25])
{
    sort(&p[1], &p[2]);
    sort(&p[0], &p[1]);
    sort(&p[1], &p[2]);
    sort(&p[4], &p[5]);
    sort(&p[3], &p[4]);
    sort(&p[4], &p[5]);
    sort(&p[0], &p[3]);
    sort(&p[2], &p[5]);
    sort(&p[2], &p[3]);
    sort(&p[1], &p[4]);
    sort(&p[1], &p[2]);
    sort(&p[3], &p[4]);
    sort(&p[7], &p[8]);
    sort(&p[6], &p[7]);
    sort(&p[7], &p[8]);
    sort(&p[10], &p[11]);
    sort(&p[9], &p[10]);
    sort(&p[10], &p[11]);
    sort(&p[6], &p[9]);
    sort(&p[8], &p[11]);
    sort(&p[8], &p[9]);
    sort(&p[7], &p[10]);
    sort(&p[7], &p[8]);
    sort(&p[9], &p[10]);
    sort(&p[0], &p[6]);
    sort(&p[4], &p[10]);
    sort(&p[4], &p[6]);
    sort(&p[2], &p[8]);
    sort(&p[2], &p[4]);
    sort(&p[6], &p[8]);
    sort(&p[1], &p[7]);
    sort(&p[5], &p[11]);
    sort(&p[5], &p[7]);
    sort(&p[3], &p[9]);
    sort(&p[3], &p[5]);
    sort(&p[7], &p[9]);
    sort(&p[1], &p[2]);
    sort(&p[3], &p[4]);
    sort(&p[5], &p[6]);
    sort(&p[7], &p[8]);
    sort(&p[9], &p[10]);
    sort(&p[13], &p[14]);
    sort(&p[12], &p[13]);
    sort(&p[13], &p[14]);
    sort(&p[16], &p[17]);
    sort(&p[15], &p[16]);
    sort(&p[16], &p[17]);
    sort(&p[12], &p[15]);
    sort(&p[14], &p[17]);
    sort(&p[14], &p[15]);
    sort(&p[13], &p[16]);
    sort(&p[13], &p[14]);
    sort(&p[15], &p[16]);
    sort(&p[19], &p[20]);
    sort(&p[18], &p[19]);
    sort(&p[19], &p[20]);
    sort(&p[21], &p[22]);
    sort(&p[23], &p[24]);
    sort(&p[21], &p[23]);
    sort(&p[22], &p[24]);
    sort(&p[22], &p[23]);
    sort(&p[18], &p[21]);
    sort(&p[20], &p[23]);
    sort(&p[20], &p[21]);
    sort(&p[19], &p[22]);
    sort(&p[22], &p[24]);
    sort(&p[19], &p[20]);
    sort(&p[21], &p[22]);
    sort(&p[23], &p[24]);
    sort(&p[12], &p[18]);
    sort(&p[16], &p[22]);
    sort(&p[16], &p[18]);
    sort(&p[14], &p[20]);
    sort(&p[20], &p[24]);
    sort(&p[14], &p[16]);
    sort(&p[18], &p[20]);
    sort(&p[22], &p[24]);
    sort(&p[13], &p[19]);
    sort(&p[17], &p[23]);
    sort(&p[17], &p[19]);
    sort(&p[15], &p[21]);
    sort(&p[15], &p[17]);
    sort(&p[19], &p[21]);
    sort(&p[13], &p[14]);
    sort(&p[15], &p[16]);
    sort(&p[17], &p[18]);
    sort(&p[19], &p[20]);
    sort(&p[21], &p[22]);
    sort(&p[23], &p[24]);
    sort(&p[0], &p[12]);
    sort(&p[8], &p[20]);
    sort(&p[8], &p[12]);
    sort(&p[4], &p[16]);
    sort(&p[16], &p[24]);
    sort(&p[12], &p[16]);
    sort(&p[2], &p[14]);
    sort(&p[10], &p[22]);
    sort(&p[10], &p[14]);
    sort(&p[6], &p[18]);
    sort(&p[6], &p[10]);
    sort(&p[10], &p[12]);
    sort(&p[1], &p[13]);
    sort(&p[9], &p[21]);
    sort(&p[9], &p[13]);
    sort(&p[5], &p[17]);
    sort(&p[13], &p[17]);
    sort(&p[3], &p[15]);
    sort(&p[11], &p[23]);
    sort(&p[11], &p[15]);
    sort(&p[7], &p[19]);
    sort(&p[7], &p[11]);
    sort(&p[11], &p[13]);
    sort(&p[11], &p[12]);
}


static void sort25_min(uint8x8_t p[25])
{
    sort_min(&p[0], &p[1]);
    sort_min(&p[0], &p[2]);
    sort_min(&p[0], &p[3]);
    sort_min(&p[0], &p[4]);
    sort_min(&p[0], &p[5]);
    sort_min(&p[0], &p[6]);
    sort_min(&p[0], &p[7]);
    sort_min(&p[0], &p[8]);
    sort_min(&p[0], &p[9]);
    sort_min(&p[0], &p[10]);
    sort_min(&p[0], &p[11]);
    sort_min(&p[0], &p[12]);
    sort_min(&p[0], &p[13]);
    sort_min(&p[0], &p[14]);
    sort_min(&p[0], &p[15]);
    sort_min(&p[0], &p[16]);
    sort_min(&p[0], &p[17]);
    sort_min(&p[0], &p[18]);
    sort_min(&p[0], &p[19]);
    sort_min(&p[0], &p[20]);
    sort_min(&p[0], &p[21]);
    sort_min(&p[0], &p[22]);
    sort_min(&p[0], &p[23]);
    sort_min(&p[0], &p[24]);
}

static void sort25_max(uint8x8_t p[25])
{
    sort_max(&p[0], &p[1]);
    sort_max(&p[0], &p[2]);
    sort_max(&p[0], &p[3]);
    sort_max(&p[0], &p[4]);
    sort_max(&p[0], &p[5]);
    sort_max(&p[0], &p[6]);
    sort_max(&p[0], &p[7]);
    sort_max(&p[0], &p[8]);
    sort_max(&p[0], &p[9]);
    sort_max(&p[0], &p[10]);
    sort_max(&p[0], &p[11]);
    sort_max(&p[0], &p[12]);
    sort_max(&p[0], &p[13]);
    sort_max(&p[0], &p[14]);
    sort_max(&p[0], &p[15]);
    sort_max(&p[0], &p[16]);
    sort_max(&p[0], &p[17]);
    sort_max(&p[0], &p[18]);
    sort_max(&p[0], &p[19]);
    sort_max(&p[0], &p[20]);
    sort_max(&p[0], &p[21]);
    sort_max(&p[0], &p[22]);
    sort_max(&p[0], &p[23]);
    sort_max(&p[0], &p[24]);
}

static void filter_cross_3x3_neon(struct src_ptr src, vx_uint8* dst, vx_int32 low_x, vx_int32 high_x, vx_enum function)
{
    vx_uint32 x;
    for (x = low_x; x < high_x; x += 8)
    {
        const uint8x8_t top_data = vld1_u8(src.top_src);
        const uint8x16_t mid_data = vld1q_u8(src.mid_src);
        const uint8x8_t bot_data = vld1_u8(src.bot_src);

        uint8x8_t p0 = top_data;
        uint8x8_t p1 = vget_low_u8(mid_data);
        uint8x8_t p2 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 1);
        uint8x8_t p3 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 2);
        uint8x8_t p4 = bot_data;

        switch (function)
        {
            /* minimal value */
            case VX_NONLINEAR_FILTER_MIN:
            {
                sort5_min(&p0, &p1, &p2, &p3, &p4);
                vst1_u8(dst, p0);
                break;
            }
            /* maximum value */
            case VX_NONLINEAR_FILTER_MAX:
            {
                sort5_max(&p0, &p1, &p2, &p3, &p4);
                vst1_u8(dst, p0);
                break;
            }
            /* pick the middle value */
            case VX_NONLINEAR_FILTER_MEDIAN:
            {
                sort5_mid(&p0, &p1, &p2, &p3, &p4);
                vst1_u8(dst, p2);
                break;
            }
        }

        dst += 8;
        src.top_src += 8;
        src.mid_src += 8;
        src.bot_src += 8;
    }
}


static void filter_cross_3x3(vx_tile_ex_t *in, vx_tile_ex_t *out, vx_enum function, vx_size ry0)
{
    vx_uint32 y;

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = 0;
    vx_uint32 high_x = out->tile_block.width;

    vx_int32 src_stride_y = in->addr->stride_y;
    vx_int32 dst_stride_y = out->addr->stride_y;

    struct src_ptr src;

    if (ry0 == 0)
    {
        if (high_y == out->image.height)
        {
            high_y = high_y - 2;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + y * dst_stride_y;
            src.top_src = (vx_uint8 *)src_base + 1 + (y)* src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y + 1) * src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + 1 + (y + 2) * src_stride_y;

            filter_cross_3x3_neon(src, dst, low_x, high_x, function);
        }
    }
    else 
    {
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
            vx_uint8* dst = (vx_uint8 *)dst_base + 1 + y * dst_stride_y;
            src.top_src = (vx_uint8 *)src_base + 1 + (y - 1) * src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + 1 + (y + 1) * src_stride_y;

            filter_cross_3x3_neon(src, dst, low_x, high_x, function);
        }
    }
}


static void filter_box_3x3_neon(struct src_ptr src, vx_uint8* dst, vx_int32 low_x, vx_int32 high_x, vx_enum function)
{
    vx_uint32 x;
    for (x = low_x; x < high_x; x += 8)
    {
        const uint8x16_t top_data = vld1q_u8(src.top_src);
        const uint8x16_t mid_data = vld1q_u8(src.mid_src);
        const uint8x16_t bot_data = vld1q_u8(src.bot_src);

        uint8x8_t p0 = vget_low_u8(top_data);
        uint8x8_t p1 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 1);
        uint8x8_t p2 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 2);
        uint8x8_t p3 = vget_low_u8(mid_data);
        uint8x8_t p4 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 1);
        uint8x8_t p5 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 2);
        uint8x8_t p6 = vget_low_u8(bot_data);
        uint8x8_t p7 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 1);
        uint8x8_t p8 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 2);

        switch (function)
        {
            /* minimal value */
            case VX_NONLINEAR_FILTER_MIN:
            {
                sort9_min(&p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
                vst1_u8(dst, p0);
                break;
            }
            /* maximum value */
            case VX_NONLINEAR_FILTER_MAX:
            {
                sort9_max(&p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
                vst1_u8(dst, p0);
                break;
            }
            /* pick the middle value */
            case VX_NONLINEAR_FILTER_MEDIAN:
            {
                sort9_mid(&p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
                vst1_u8(dst, p4);
                break;
            }
        }

        dst += 8;
        src.top_src += 8;
        src.mid_src += 8;
        src.bot_src += 8;
    }
}


static void filter_box_3x3(vx_tile_ex_t *in, vx_tile_ex_t *out, vx_enum function, vx_size ry0)
{
    vx_uint32 y;

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = 0;
    vx_uint32 high_x = out->tile_block.width;

    vx_int32 src_stride_y = in->addr->stride_y;
    vx_int32 dst_stride_y = out->addr->stride_y;

    struct src_ptr src;

    if (ry0 == 0)
    {
        if (high_y == out->image.height)
        {
            high_y = high_y - 2;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + y * dst_stride_y;
            src.top_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y + 1) * src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + (y + 2) * src_stride_y;

            filter_box_3x3_neon(src, dst, low_x, high_x, function);
        }
    }
    else
    {
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
            vx_uint8* dst = (vx_uint8 *)dst_base + 1 + y * dst_stride_y;
            src.top_src = (vx_uint8 *)src_base + (y - 1) * src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + (y + 1) * src_stride_y;

            filter_box_3x3_neon(src, dst, low_x, high_x, function);
        }
    }
}


static void filter_cross_5x5_neon(struct src_ptr src, vx_uint8* dst, vx_int32 low_x, vx_int32 high_x, vx_enum function)
{
    vx_uint32 x;
    for (x = low_x; x < high_x; x += 8)
    {
        const uint8x8_t  top2_data = vld1_u8(src.top2_src);
        const uint8x8_t  top_data = vld1_u8(src.top_src);
        const uint8x16_t mid_data = vld1q_u8(src.mid_src);
        const uint8x8_t  bot_data = vld1_u8(src.bot_src);
        const uint8x8_t  bot2_data = vld1_u8(src.bot2_src);

        uint8x8_t p0 = top2_data;
        uint8x8_t p1 = top_data;
        uint8x8_t p2 = vget_low_u8(mid_data);
        uint8x8_t p3 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 1);
        uint8x8_t p4 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 2);
        uint8x8_t p5 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 3);
        uint8x8_t p6 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 4);
        uint8x8_t p7 = bot_data;
        uint8x8_t p8 = bot2_data;

        switch (function)
        {
            /* minimal value */
        case VX_NONLINEAR_FILTER_MIN:
        {
            sort9_min(&p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
            vst1_u8(dst, p0);
            break;
        }
        /* maximum value */
        case VX_NONLINEAR_FILTER_MAX:
        {
            sort9_max(&p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
            vst1_u8(dst, p0);
            break;
        }
        /* pick the middle value */
        case VX_NONLINEAR_FILTER_MEDIAN:
        {
            sort9_mid(&p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
            vst1_u8(dst, p4);
            break;
        }
        }

        dst += 8;
        src.top2_src += 8;
        src.top_src += 8;
        src.mid_src += 8;
        src.bot_src += 8;
        src.bot2_src += 8;
    }
}


static void* filter_cross_5x5(vx_tile_ex_t *in, vx_tile_ex_t *out, vx_enum function, vx_size ry0)
{
    vx_uint32 y;
    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = 0;
    vx_uint32 high_x = out->tile_block.width;

    vx_int32 src_stride_y = in->addr->stride_y;
    vx_int32 dst_stride_y = out->addr->stride_y;

    struct src_ptr src;

    if (ry0 == 1)
    {
        if (high_y == out->image.height)
        {
            high_y = high_y - 3;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + 1 + y * dst_stride_y;
            src.top2_src = (vx_uint8 *)src_base + 2 + (y - 1)* src_stride_y;
            src.top_src = (vx_uint8 *)src_base + 2 + (y)* src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y + 1)* src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + 2 + (y + 2)* src_stride_y;
            src.bot2_src = (vx_uint8 *)src_base + 2 + (y + 3)* src_stride_y;

            filter_cross_5x5_neon(src, dst, low_x, high_x, function);
        }
    }
    else
    {
        if (low_y == 0)
        {
            low_y = 2;
        }
        if (high_y == out->image.height)
        {
            high_y = high_y - 2;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + 2 + y * dst_stride_y;
            src.top2_src = (vx_uint8 *)src_base + 2 + (y - 2) * src_stride_y;
            src.top_src = (vx_uint8 *)src_base + 2 + (y - 1) * src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + 2 + (y + 1)* src_stride_y;
            src.bot2_src = (vx_uint8 *)src_base + 2 + (y + 2)* src_stride_y;

            filter_cross_5x5_neon(src, dst, low_x, high_x, function);
        }
    }
}


static void filter_box_5x5_neon(struct src_ptr src, vx_uint8* dst, vx_int32 low_x, vx_int32 high_x, vx_enum function)
{
    vx_uint32 x;
    for (x = low_x; x < high_x; x += 8)
    {
        const uint8x16_t top2_data = vld1q_u8(src.top2_src);
        const uint8x16_t top_data = vld1q_u8(src.top_src);
        const uint8x16_t mid_data = vld1q_u8(src.mid_src);
        const uint8x16_t bot_data = vld1q_u8(src.bot_src);
        const uint8x16_t bot2_data = vld1q_u8(src.bot2_src);

        const uint8x8_t d[] =
        {
            vget_low_u8(top2_data),
            vget_high_u8(top2_data),
            vget_low_u8(top_data),
            vget_high_u8(top_data),
            vget_low_u8(mid_data),
            vget_high_u8(mid_data),
            vget_low_u8(bot_data),
            vget_high_u8(bot_data),
            vget_low_u8(bot2_data),
            vget_high_u8(bot2_data)
        };

        uint8x8_t p[25];
        for (vx_uint32 i = 0; i < 5; ++i)
        {
            const vx_uint32 idx_d = i * 2;
            const vx_uint32 idx_p = i * 5;

            p[idx_p] = d[idx_d];
            p[idx_p + 1] = vext_u8(d[idx_d], d[idx_d + 1], 1);
            p[idx_p + 2] = vext_u8(d[idx_d], d[idx_d + 1], 2);
            p[idx_p + 3] = vext_u8(d[idx_d], d[idx_d + 1], 3);
            p[idx_p + 4] = vext_u8(d[idx_d], d[idx_d + 1], 4);
        }

        switch (function)
        {
            /* minimal value */
        case VX_NONLINEAR_FILTER_MIN:
        {
            sort25_min(p);
            vst1_u8(dst, p[0]);
            break;
        }
        /* maximum value */
        case VX_NONLINEAR_FILTER_MAX:
        {
            sort25_max(p);
            vst1_u8(dst, p[0]);
            break;
        }
        /* pick the middle value */
        case VX_NONLINEAR_FILTER_MEDIAN:
        {
            sort25_mid(p);
            vst1_u8(dst, p[12]);
            break;
        }
        }

        dst += 8;
        src.top2_src += 8;
        src.top_src += 8;
        src.mid_src += 8;
        src.bot_src += 8;
        src.bot2_src += 8;
    }
}


static void* filter_box_5x5(vx_tile_ex_t *in, vx_tile_ex_t *out, vx_enum function, vx_size ry0)
{
    vx_uint32 y;
    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = 0;
    vx_uint32 high_x = out->tile_block.width;

    vx_int32 src_stride_y = in->addr->stride_y;
    vx_int32 dst_stride_y = out->addr->stride_y;

    struct src_ptr src;

    if (ry0 == 1)
    {
        if (high_y == out->image.height)
        {
            high_y = high_y - 3;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + 1 + y * dst_stride_y;
            src.top2_src = (vx_uint8 *)src_base + (y - 1)* src_stride_y;
            src.top_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y + 1) * src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + (y + 2) * src_stride_y;
            src.bot2_src = (vx_uint8 *)src_base + (y + 3) * src_stride_y;

            filter_box_5x5_neon(src, dst, low_x, high_x, function);
        }
    }
    else
    {
        if (low_y == 0)
        {
            low_y = 2;
        }
        if (high_y == out->image.height)
        {
            high_y = high_y - 2;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + 2 + y * dst_stride_y;
            src.top2_src = (vx_uint8 *)src_base + (y - 2) * src_stride_y;
            src.top_src = (vx_uint8 *)src_base + (y - 1) * src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + (y + 1) * src_stride_y;
            src.bot2_src = (vx_uint8 *)src_base + (y + 2) * src_stride_y;

            filter_box_5x5_neon(src, dst, low_x, high_x, function);
        }
    }
}


static void filter_disk_5x5_neon(struct src_ptr src, vx_uint8* dst, vx_int32 low_x, vx_int32 high_x, vx_enum function)
{
    vx_uint32 x;
    const uint8x16_t zero = vdupq_n_u8(0);
    for (x = low_x; x < high_x; x += 8)
    {
        const uint8x16_t top2_data = vextq_u8(vld1q_u8(src.top2_src), zero, 1);
        const uint8x16_t top_data = vld1q_u8(src.top_src);
        const uint8x16_t mid_data = vld1q_u8(src.mid_src);
        const uint8x16_t bot_data = vld1q_u8(src.bot_src);
        const uint8x16_t bot2_data = vextq_u8(vld1q_u8(src.bot2_src), zero, 1);

        uint8x8_t d[] =
        {
            vget_low_u8(top2_data),
            vget_high_u8(top2_data),
            vget_low_u8(top_data),
            vget_high_u8(top_data),
            vget_low_u8(mid_data),
            vget_high_u8(mid_data),
            vget_low_u8(bot_data),
            vget_high_u8(bot_data),
            vget_low_u8(bot2_data),
            vget_high_u8(bot2_data)
        };

        uint8x8_t p[21];
        p[0] = d[0];
        p[1] = vext_u8(d[0], d[1], 1);
        p[2] = vext_u8(d[0], d[1], 2);
        p[18] = d[8];
        p[19] = vext_u8(d[8], d[9], 1);
        p[20] = vext_u8(d[8], d[9], 2);

        for (vx_uint32 i = 0; i < 3; ++i)
        {
            const vx_uint32 idx_d = 2 + i * 2;
            const vx_uint32 idx_p = 3 + i * 5;

            p[idx_p] = d[idx_d];
            p[idx_p + 1] = vext_u8(d[idx_d], d[idx_d + 1], 1);
            p[idx_p + 2] = vext_u8(d[idx_d], d[idx_d + 1], 2);
            p[idx_p + 3] = vext_u8(d[idx_d], d[idx_d + 1], 3);
            p[idx_p + 4] = vext_u8(d[idx_d], d[idx_d + 1], 4);
        }

        switch (function)
        {
            /* minimal value */
        case VX_NONLINEAR_FILTER_MIN:
        {
            sort21_min(p);
            vst1_u8(dst, p[0]);
            break;
        }
        /* maximum value */
        case VX_NONLINEAR_FILTER_MAX:
        {
            sort21_max(p);
            vst1_u8(dst, p[0]);
            break;
        }
        /* pick the middle value */
        case VX_NONLINEAR_FILTER_MEDIAN:
        {
            sort21_mid(p);
            vst1_u8(dst, p[10]);
            break;
        }
        }

        dst += 8;
        src.top2_src += 8;
        src.top_src += 8;
        src.mid_src += 8;
        src.bot_src += 8;
        src.bot2_src += 8;
    }
}


static void* filter_disk_5x5(vx_tile_ex_t *in, vx_tile_ex_t *out, vx_enum function, vx_size ry0)
{
    vx_uint32 y;
    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = 0;
    vx_uint32 high_x = out->tile_block.width;

    vx_int32 src_stride_y = in->addr->stride_y;
    vx_int32 dst_stride_y = out->addr->stride_y;

    struct src_ptr src;

    if (ry0 == 1)
    {
        if (high_y == out->image.height)
        {
            high_y = high_y - 3;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + 1 + y * dst_stride_y;
            src.top2_src = (vx_uint8 *)src_base + (y - 1)* src_stride_y;
            src.top_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y + 1) * src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + (y + 2) * src_stride_y;
            src.bot2_src = (vx_uint8 *)src_base + (y + 3) * src_stride_y;

            filter_disk_5x5_neon(src, dst, low_x, high_x, function);
        }
    }
    else
    {
        if (low_y == 0)
        {
            low_y = 2;
        }
        if (high_y == out->image.height)
        {
            high_y = high_y - 2;
        }
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8* dst = (vx_uint8 *)dst_base + 2 + y * dst_stride_y;
            src.top2_src = (vx_uint8 *)src_base + (y - 2) * src_stride_y;
            src.top_src = (vx_uint8 *)src_base + (y - 1) * src_stride_y;
            src.mid_src = (vx_uint8 *)src_base + (y)* src_stride_y;
            src.bot_src = (vx_uint8 *)src_base + (y + 1) * src_stride_y;
            src.bot2_src = (vx_uint8 *)src_base + (y + 2) * src_stride_y;

            filter_disk_5x5_neon(src, dst, low_x, high_x, function);
        }
    }
}

void NonLinearFilter_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;
    vx_enum *func = (vx_enum *)parameters[0];
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[1];
    vx_tile_matrix_t *mask = (vx_tile_matrix_t *)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];

    vx_size ry0 = mask->origin.y;

    vx_int32 count_mask = 0;
    vx_int32 mask_index = 0;

    for (vx_uint32 r = 0; r < mask->rows; ++r)
    {
        for (vx_uint32 c = 0; c < mask->columns; ++c, ++mask_index)
        {
            if (mask->m[mask_index])
            {
                ++count_mask;
            }
        }
    }

    switch (mask->rows)
    {
        case 3:   // mask = 3x3
        {
            if (count_mask == 5)
                filter_cross_3x3(in, out, *func, ry0);
            else // count_mask = 9
                filter_box_3x3(in, out, *func, ry0);

            break;
        }

        case 5:   // mask = 5x5
        {
            if (count_mask == 9)
                filter_cross_5x5(in, out, *func, ry0);
            else if (count_mask == 21)
                filter_disk_5x5(in, out, *func, ry0);
            else  // count_mask = 25
                filter_box_5x5(in, out, *func, ry0);
            break;
        }
    }
}


// helpers
static vx_int32 vx_uint8_compare(const void *p1, const void *p2)
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


static vx_uint32 readMaskedRectangle_U8(const void *base, const vx_imagepatch_addressing_t *addr, vx_uint32 center_x, vx_uint32 center_y,
                                        vx_uint32 left, vx_uint32 top, vx_uint32 right, vx_uint32 bottom, vx_uint8 *mask, vx_uint8 *destination)
{
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 mask_index = 0;
    vx_uint32 dest_index = 0;

    for (ky = -(vx_int32)top; ky <= (vx_int32)bottom; ++ky)
    {
        vx_int32 y = (vx_int32)(center_y + ky);
        y = y < 0 ? 0 : y >= height ? height - 1 : y;

        for (kx = -(vx_int32)left; kx <= (vx_int32)right; ++kx, ++mask_index)
        {
            vx_int32 x = (vx_int32)(center_x + kx);
            x = x < 0 ? 0 : x >= width ? width - 1 : x;
            if (mask[mask_index])
                ((vx_uint8*)destination)[dest_index++] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
        }
    }

    return dest_index;
}


#define NonLinearFilter(low_y, high_y, low_x)                                                           \
    for (y = low_y; y < high_y; y++)                                                                    \
    {                                                                                                   \
        for (x = low_x; x < high_x; x++)                                                                \
        {                                                                                               \
            vx_uint8 *dst = (vx_uint8 *)dst_base + y * out->addr->stride_y + x * out->addr->stride_x;   \
            vx_uint32 count = (vx_uint32)readMaskedRectangle_U8(src_base, in->addr, x, y, (vx_uint32)rx0, (vx_uint32)ry0, (vx_uint32)rx1, (vx_uint32)ry1, mask->m, v);                                                                            \
                                                                                                        \
            qsort(v, count, sizeof(vx_uint8), vx_uint8_compare);                                        \
                                                                                                        \
            switch(*func)                                                                               \
            {                                                                                           \
                case VX_NONLINEAR_FILTER_MIN : *dst = v[0]; break;                                      \
                case VX_NONLINEAR_FILTER_MAX : *dst = v[count - 1]; break;                              \
                case VX_NONLINEAR_FILTER_MEDIAN : *dst = v[count / 2]; break;                           \
            }                                                                                           \
        }                                                                                               \
    }


static vx_uint32 readMaskedRectangle_U1(const void *base,
       const vx_imagepatch_addressing_t *addr,
       vx_uint32 center_x,
       vx_uint32 center_y,
       vx_uint32 left,
       vx_uint32 top,
       vx_uint32 right,
       vx_uint32 bottom,
       vx_uint8 *mask,
       vx_uint8 *destination,
       vx_uint32 border_x_start)
{
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    vx_uint16 stride_x_bits = addr->stride_x_bits;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 mask_index = 0;
    vx_uint32 dest_index = 0;

    // kx, ky - kernel x and y
    for (ky = -(int32_t)top; ky <= (int32_t)bottom; ++ky)
    {
        vx_int32 y = (vx_int32)(center_y + ky);
        y = y < 0 ? 0 : y >= height ? height - 1 : y;

        for (kx = -(int32_t)left; kx <= (int32_t)right; ++kx, ++mask_index)
        {
            vx_int32 x = (int32_t)(center_x + kx);
            x = x < (int32_t)border_x_start ? (int32_t)border_x_start : x >= width ? width - 1 : x;
            if (mask[mask_index])
            {
                ((vx_uint8*)destination)[dest_index++] =
                    ( *(vx_uint8*)(ptr + y*stride_y + (x*stride_x_bits) / 8) & (1 << (x % 8)) ) >> (x % 8);
            }
        }
    }

    return dest_index;
}

void NonLinearFilter_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;
    vx_enum *func = (vx_enum *)parameters[0];
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[1];
    vx_tile_matrix_t *mask = (vx_tile_matrix_t *)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];

    vx_uint8 v[C_MAX_NONLINEAR_DIM * C_MAX_NONLINEAR_DIM];
    vx_uint8 res_val;

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    vx_size rx0 = mask->origin.x;
    vx_size ry0 = mask->origin.y;
    vx_size rx1 = mask->columns - mask->origin.x - 1;
    vx_size ry1 = mask->rows - mask->origin.y - 1;

    if (in->is_U1 == 0)
    {
        if (low_y == 0 && low_x == 0)
        {
            NonLinearFilter(low_y, high_y, low_x)
        }
        else
        {
            NonLinearFilter(0, low_y, low_x)

            src_base = in->base[0];
            dst_base = out->base[0];
            NonLinearFilter(low_y, high_y, 0)
        }
    }
    else
    {
        void *src_base = in->base[0];                                                           
        void *dst_base = out->base[0];    
        vx_uint32 shift_x_u1 = in->rect.start_x % 8;                                                         
        high_x = high_x - shift_x_u1;                                                                  
                                         
        low_x  += (vx_uint32)rx0;
        low_y  += (vx_uint32)ry0;
        high_x -= (vx_uint32)rx1;
        high_y -= (vx_uint32)ry1;                                                      
        for (y = low_y; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x++)
            {
                vx_uint32 xShftd = x + shift_x_u1;      // Bit-shift for U1 valid region start
                vx_uint8 *dst_ptr = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xShftd, y, out->addr);
                vx_int32 count = (vx_int32)readMaskedRectangle_U1(src_base, in->addr, xShftd, y, (vx_uint32)rx0, (vx_uint32)ry0, (vx_uint32)rx1, (vx_uint32)ry1, mask->m, v, shift_x_u1);

                qsort(v, count, sizeof(vx_uint8), vx_uint8_compare);

                switch (*func)
                {
                case VX_NONLINEAR_FILTER_MIN:    res_val = v[0];         break; /* minimal value */
                case VX_NONLINEAR_FILTER_MAX:    res_val = v[count - 1]; break; /* maximum value */
                case VX_NONLINEAR_FILTER_MEDIAN: res_val = v[count / 2]; break; /* pick the middle value */
                }

                *dst_ptr = (*dst_ptr & ~(1 << (xShftd % 8))) | (res_val << (xShftd % 8));
            }
        }
    }
}
