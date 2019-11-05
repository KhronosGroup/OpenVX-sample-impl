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

#include "vx_interface.h"

#include "vx_internal.h"

#include <arm_neon.h>

#include "tiling.h"

#define MIN(a,b)    (((a) < (b)) ? (a) : (b))

static vx_int32 * alignPtr(vx_int32* ptr, size_t n)
{
    return (vx_int32 *)(((size_t)ptr + n-1) & -n);
}

static vx_float32 * alignPtr_f(vx_float32* ptr, size_t n)
{
    return (vx_float32 *)(((size_t)ptr + n-1) & -n);
}


static void remapNearestNeighborConst(const size_t height,
                                      const size_t width,
                                      const vx_uint8 * srcBase,
                                      const vx_int32 * map,
                                      vx_uint8 * dstBase, ptrdiff_t dstStride,
                                      vx_uint8 borderValue)
{
    for (size_t y = 0; y < height; ++y)
    {
        const vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(map) + y * width * sizeof(vx_int32));
        vx_uint8 * dst_row = (vx_uint8 *)((vx_int8 *)dstBase + y * dstStride);

        for (size_t x = 0; x < width; ++x)
        {
            vx_int32 src_idx = map_row[x];
            dst_row[x] = src_idx >= 0 ? srcBase[map_row[x]] : borderValue;
        }
    }
}

static void remapLinearConst(const size_t height,
                             const size_t width,
                             const vx_uint8 * srcBase,
                             const vx_int32 * map,
                             const vx_float32 * coeffs,
                             vx_uint8 * dstBase, ptrdiff_t dstStride,
                             vx_uint8 borderValue)
{
    int16x8_t v_zero16 = vdupq_n_s16(0);

    for (size_t y = 0; y < height; ++y)
    {
        const vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(map) + y * width * sizeof(vx_int32) * 4);
        const vx_float32 * coeff_row = (vx_float32 *)((vx_int8 *)(coeffs) + y * width * sizeof(vx_float32) * 2);

        vx_uint8 * dst_row = (vx_uint8 *)((vx_int8 *)(dstBase) + y * dstStride);

        size_t x = 0;

        for ( ; x + 8 < width; x += 8)
        {
            int16x8_t v_src00 = vsetq_lane_s16(map_row[(x << 2)] >= 0 ? srcBase[map_row[(x << 2)]] : borderValue, v_zero16, 0);
            v_src00 = vsetq_lane_s16(map_row[(x << 2) +  4] >= 0 ? srcBase[map_row[(x << 2) +  4]] : borderValue, v_src00, 1);
            v_src00 = vsetq_lane_s16(map_row[(x << 2) +  8] >= 0 ? srcBase[map_row[(x << 2) +  8]] : borderValue, v_src00, 2);
            v_src00 = vsetq_lane_s16(map_row[(x << 2) + 12] >= 0 ? srcBase[map_row[(x << 2) + 12]] : borderValue, v_src00, 3);
            v_src00 = vsetq_lane_s16(map_row[(x << 2) + 16] >= 0 ? srcBase[map_row[(x << 2) + 16]] : borderValue, v_src00, 4);
            v_src00 = vsetq_lane_s16(map_row[(x << 2) + 20] >= 0 ? srcBase[map_row[(x << 2) + 20]] : borderValue, v_src00, 5);
            v_src00 = vsetq_lane_s16(map_row[(x << 2) + 24] >= 0 ? srcBase[map_row[(x << 2) + 24]] : borderValue, v_src00, 6);
            v_src00 = vsetq_lane_s16(map_row[(x << 2) + 28] >= 0 ? srcBase[map_row[(x << 2) + 28]] : borderValue, v_src00, 7);

            int16x8_t v_src01 = vsetq_lane_s16(map_row[(x << 2) + 1] >= 0 ? srcBase[map_row[(x << 2) + 1]] : borderValue, v_zero16, 0);
            v_src01 = vsetq_lane_s16(map_row[(x << 2) +  5] >= 0 ? srcBase[map_row[(x << 2) +  5]] : borderValue, v_src01, 1);
            v_src01 = vsetq_lane_s16(map_row[(x << 2) +  9] >= 0 ? srcBase[map_row[(x << 2) +  9]] : borderValue, v_src01, 2);
            v_src01 = vsetq_lane_s16(map_row[(x << 2) + 13] >= 0 ? srcBase[map_row[(x << 2) + 13]] : borderValue, v_src01, 3);
            v_src01 = vsetq_lane_s16(map_row[(x << 2) + 17] >= 0 ? srcBase[map_row[(x << 2) + 17]] : borderValue, v_src01, 4);
            v_src01 = vsetq_lane_s16(map_row[(x << 2) + 21] >= 0 ? srcBase[map_row[(x << 2) + 21]] : borderValue, v_src01, 5);
            v_src01 = vsetq_lane_s16(map_row[(x << 2) + 25] >= 0 ? srcBase[map_row[(x << 2) + 25]] : borderValue, v_src01, 6);
            v_src01 = vsetq_lane_s16(map_row[(x << 2) + 29] >= 0 ? srcBase[map_row[(x << 2) + 29]] : borderValue, v_src01, 7);

            int16x8_t v_src10 = vsetq_lane_s16(map_row[(x << 2) + 2] >= 0 ? srcBase[map_row[(x << 2) + 2]] : borderValue, v_zero16, 0);
            v_src10 = vsetq_lane_s16(map_row[(x << 2) +  6] >= 0 ? srcBase[map_row[(x << 2) +  6]] : borderValue, v_src10, 1);
            v_src10 = vsetq_lane_s16(map_row[(x << 2) + 10] >= 0 ? srcBase[map_row[(x << 2) + 10]] : borderValue, v_src10, 2);
            v_src10 = vsetq_lane_s16(map_row[(x << 2) + 14] >= 0 ? srcBase[map_row[(x << 2) + 14]] : borderValue, v_src10, 3);
            v_src10 = vsetq_lane_s16(map_row[(x << 2) + 18] >= 0 ? srcBase[map_row[(x << 2) + 18]] : borderValue, v_src10, 4);
            v_src10 = vsetq_lane_s16(map_row[(x << 2) + 22] >= 0 ? srcBase[map_row[(x << 2) + 22]] : borderValue, v_src10, 5);
            v_src10 = vsetq_lane_s16(map_row[(x << 2) + 26] >= 0 ? srcBase[map_row[(x << 2) + 26]] : borderValue, v_src10, 6);
            v_src10 = vsetq_lane_s16(map_row[(x << 2) + 30] >= 0 ? srcBase[map_row[(x << 2) + 30]] : borderValue, v_src10, 7);

            int16x8_t v_src11 = vsetq_lane_s16(map_row[(x << 2) + 3] >= 0 ? srcBase[map_row[(x << 2) + 3]] : borderValue, v_zero16, 0);
            v_src11 = vsetq_lane_s16(map_row[(x << 2) +  7] >= 0 ? srcBase[map_row[(x << 2) +  7]] : borderValue, v_src11, 1);
            v_src11 = vsetq_lane_s16(map_row[(x << 2) + 11] >= 0 ? srcBase[map_row[(x << 2) + 11]] : borderValue, v_src11, 2);
            v_src11 = vsetq_lane_s16(map_row[(x << 2) + 15] >= 0 ? srcBase[map_row[(x << 2) + 15]] : borderValue, v_src11, 3);
            v_src11 = vsetq_lane_s16(map_row[(x << 2) + 19] >= 0 ? srcBase[map_row[(x << 2) + 19]] : borderValue, v_src11, 4);
            v_src11 = vsetq_lane_s16(map_row[(x << 2) + 23] >= 0 ? srcBase[map_row[(x << 2) + 23]] : borderValue, v_src11, 5);
            v_src11 = vsetq_lane_s16(map_row[(x << 2) + 27] >= 0 ? srcBase[map_row[(x << 2) + 27]] : borderValue, v_src11, 6);
            v_src11 = vsetq_lane_s16(map_row[(x << 2) + 31] >= 0 ? srcBase[map_row[(x << 2) + 31]] : borderValue, v_src11, 7);

            // first part
            float32x4_t v_src00_f = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src00)));
            float32x4_t v_src10_f = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src10)));

            float32x4x2_t v_coeff = vld2q_f32(coeff_row + (x << 1));
            float32x4_t v_dst_0 = vmlaq_f32(v_src00_f, vcvtq_f32_s32(vsubl_s16(vget_low_s16(v_src01),
                                                                               vget_low_s16(v_src00))), v_coeff.val[0]);
            float32x4_t v_dst_1 = vmlaq_f32(v_src10_f, vcvtq_f32_s32(vsubl_s16(vget_low_s16(v_src11),
                                                                               vget_low_s16(v_src10))), v_coeff.val[0]);

            float32x4_t v_dst = vmlaq_f32(v_dst_0, vsubq_f32(v_dst_1, v_dst_0), v_coeff.val[1]);
            uint16x4_t v_dst0 = vmovn_u32(vcvtq_u32_f32(v_dst));

            // second part
            v_src00_f = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src00)));
            v_src10_f = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src10)));

            v_coeff = vld2q_f32(coeff_row + (x << 1) + 8);
            v_dst_0 = vmlaq_f32(v_src00_f, vcvtq_f32_s32(vsubl_s16(vget_high_s16(v_src01),
                                                                   vget_high_s16(v_src00))), v_coeff.val[0]);
            v_dst_1 = vmlaq_f32(v_src10_f, vcvtq_f32_s32(vsubl_s16(vget_high_s16(v_src11),
                                                                   vget_high_s16(v_src10))), v_coeff.val[0]);

            v_dst = vmlaq_f32(v_dst_0, vsubq_f32(v_dst_1, v_dst_0), v_coeff.val[1]);
            uint16x4_t v_dst1 = vmovn_u32(vcvtq_u32_f32(v_dst));

            // store
            vst1_u8(dst_row + x, vmovn_u16(vcombine_u16(v_dst0, v_dst1)));
        }
        for ( ; x < width; ++x)
        {
            int16_t src00 = map_row[(x << 2) + 0] >= 0 ? srcBase[map_row[(x << 2) + 0]] : borderValue;
            int16_t src01 = map_row[(x << 2) + 1] >= 0 ? srcBase[map_row[(x << 2) + 1]] : borderValue;
            int16_t src10 = map_row[(x << 2) + 2] >= 0 ? srcBase[map_row[(x << 2) + 2]] : borderValue;
            int16_t src11 = map_row[(x << 2) + 3] >= 0 ? srcBase[map_row[(x << 2) + 3]] : borderValue;

            vx_float32 dst_val_0 = (src01 - src00) * coeff_row[(x << 1)] + src00;
            vx_float32 dst_val_1 = (src11 - src10) * coeff_row[(x << 1)] + src10;
            dst_row[x] = floorf((dst_val_1 - dst_val_0) * coeff_row[(x << 1) + 1] + dst_val_0);
        }
    }
}


//BLOCK_SIZE is the same as tile_size set in "vx_remap.c"
#define BLOCK_SIZE 16

void Remap_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_remap *table = (vx_remap *)parameters[1];
    vx_scalar *stype = (vx_scalar *)parameters[2];
    vx_tile_t *out = (vx_tile_t *)parameters[3];

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_int32 policy = (vx_int32)*stype;

    vx_uint32 src_width = in->image.width;
    vx_uint32 src_height = in->image.height;
    vx_uint32 srcStride = in->addr->stride_y;

    vx_uint32 dst_width = out->image.width;
    vx_uint32 dst_height = out->image.height;
    vx_uint32 dstStride = out->addr->stride_y;

    int32x4_t v_width4 = vdupq_n_s32(src_width - 1), v_height4 = vdupq_n_s32(src_height - 1);
    int32x4_t v_step4 = vdupq_n_s32(srcStride), v_1 = vdupq_n_s32(1);
    float32x4_t v_zero4f = vdupq_n_f32(0.0f), v_one4f = vdupq_n_f32(1.0f);

    vx_uint8 borderValue = 0;

    size_t i = out->tile_y;
    size_t blockHeight = MIN(BLOCK_SIZE, dst_height - i);
    size_t j = out->tile_x;
    size_t blockWidth = MIN(BLOCK_SIZE, dst_width - j);

    size_t tableStride = (&(*table)->memory)->strides[0][VX_DIM_Y];
    vx_float32 * tableBase = (vx_float32 *)(&(*table)->memory)->ptrs[0];

    if (policy == VX_INTERPOLATION_NEAREST_NEIGHBOR)
    {
        vx_int32 _map[BLOCK_SIZE * BLOCK_SIZE + 16];
        vx_int32 * map = alignPtr(_map, 16);

        int32x4_t v_m1_4 = vdupq_n_s32(-1);
        int32x2_t v_m1_2 = vdup_n_s32(-1);
        float32x4_t v_zero4 = vdupq_n_f32(0.0f);
        float32x2_t v_zero2 = vdup_n_f32(0.0f);

        // compute table
        for (size_t y = 0; y < blockHeight; ++y)
        {
            const vx_float32 * table_row = (vx_float32 *)((vx_int8 *)(tableBase) + (i + y) * tableStride) + (j << 1);
            vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(&map[0]) + y * blockWidth * sizeof(vx_int32));

            size_t x = 0;
            for ( ; x + 8 <= blockWidth; x += 8)
            {
                float32x4x2_t v_table0 = vld2q_f32(table_row + (x << 1)),
                              v_table1 = vld2q_f32(table_row + (x << 1) + 8);

                int32x4_t v_dst_x = vcvtq_s32_f32(v_table0.val[0]);
                int32x4_t v_dst_y = vcvtq_s32_f32(v_table0.val[1]);
                uint32x4_t v_mask = vandq_u32(vandq_u32(vcgeq_f32(v_table0.val[0], v_zero4), vcleq_s32(v_dst_x, v_width4)),
                                              vandq_u32(vcgeq_f32(v_table0.val[1], v_zero4), vcleq_s32(v_dst_y, v_height4)));
                int32x4_t v_dst_index = vbslq_s32(v_mask, vmlaq_s32(v_dst_x, v_dst_y, v_step4), v_m1_4);
                vst1q_s32(map_row + x, v_dst_index);

                v_dst_x = vcvtq_s32_f32(v_table1.val[0]);
                v_dst_y = vcvtq_s32_f32(v_table1.val[1]);
                v_mask = vandq_u32(vandq_u32(vcgeq_f32(v_table1.val[0], v_zero4), vcleq_s32(v_dst_x, v_width4)),
                                   vandq_u32(vcgeq_f32(v_table1.val[1], v_zero4), vcleq_s32(v_dst_y, v_height4)));
                v_dst_index = vbslq_s32(v_mask, vmlaq_s32(v_dst_x, v_dst_y, v_step4), v_m1_4);
                vst1q_s32(map_row + x + 4, v_dst_index);
            }
        }
        vx_uint8 * dstBase = (vx_uint8 *)((vx_int8 *)dst_base + i * dstStride);
        // make remap
        remapNearestNeighborConst(blockHeight, blockWidth, src_base, &map[0], dstBase + j, dstStride, borderValue);
    }
    else if (policy == VX_INTERPOLATION_BILINEAR)
    {
        vx_int32 _map[((BLOCK_SIZE * BLOCK_SIZE) << 2) + 16];
        vx_float32 _coeffs[((BLOCK_SIZE * BLOCK_SIZE) << 1) + 16];
        vx_int32 * map = alignPtr(_map, 16);
        vx_float32 * coeffs = alignPtr_f(_coeffs, 16);

        float32x4_t v_zero4 = vdupq_n_f32(0.0f);
        int32x4_t v_m1_4 = vdupq_n_s32(-1);

        // compute table
        for (size_t y = 0; y < blockHeight; ++y)
        {
            const vx_float32 * table_row = (vx_float32 *)((vx_int8 *)(tableBase) + (i + y) * tableStride) + (j << 1);
            vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(map) + y * blockWidth * sizeof(vx_int32) * 4);
            vx_float32 * coeff_row = (vx_float32 *)((vx_int8 *)(coeffs) + y * blockWidth * sizeof(vx_float32) * 2);

            size_t x = 0;
            for ( ; x + 4 <= blockWidth; x += 4)
            {
                float32x4x2_t v_table = vld2q_f32(table_row + (x << 1));

                int32x4_t v_src_x0 = vcvtq_s32_f32(v_table.val[0]);
                int32x4_t v_src_y0 = vcvtq_s32_f32(v_table.val[1]);

                float32x4x2_t v_coeff;
                v_coeff.val[0] = vsubq_f32(v_table.val[0], vcvtq_f32_s32(v_src_x0));
                v_coeff.val[1] = vsubq_f32(v_table.val[1], vcvtq_f32_s32(v_src_y0));
                uint32x4_t v_maskx = vcltq_f32(v_coeff.val[0], v_zero4f);
                uint32x4_t v_masky = vcltq_f32(v_coeff.val[1], v_zero4f);
                v_coeff.val[0] = vbslq_f32(v_maskx, vaddq_f32(v_one4f, v_coeff.val[0]), v_coeff.val[0]);
                v_coeff.val[1] = vbslq_f32(v_masky, vaddq_f32(v_one4f, v_coeff.val[1]), v_coeff.val[1]);
                v_src_x0 = vbslq_s32(v_maskx, vsubq_s32(v_src_x0, v_1), v_src_x0);
                v_src_y0 = vbslq_s32(v_masky, vsubq_s32(v_src_y0, v_1), v_src_y0);

                int32x4_t v_src_x1 = vaddq_s32(v_src_x0, v_1);
                int32x4_t v_src_y1 = vaddq_s32(v_src_y0, v_1);

                int32x4x4_t v_dst_index;
                v_dst_index.val[0] = vmlaq_s32(v_src_x0, v_src_y0, v_step4);
                v_dst_index.val[1] = vmlaq_s32(v_src_x1, v_src_y0, v_step4);
                v_dst_index.val[2] = vmlaq_s32(v_src_x0, v_src_y1, v_step4);
                v_dst_index.val[3] = vmlaq_s32(v_src_x1, v_src_y1, v_step4);

                uint32x4_t v_mask_x0 = vandq_u32(vcgeq_f32(v_table.val[0], v_zero4), vcleq_s32(v_src_x0, v_width4));
                uint32x4_t v_mask_x1 = vandq_u32(vcgeq_f32(vaddq_f32(v_table.val[0], v_one4f), v_zero4), vcleq_s32(v_src_x1, v_width4));
                uint32x4_t v_mask_y0 = vandq_u32(vcgeq_f32(v_table.val[1], v_zero4), vcleq_s32(v_src_y0, v_height4));
                uint32x4_t v_mask_y1 = vandq_u32(vcgeq_f32(vaddq_f32(v_table.val[1], v_one4f), v_zero4), vcleq_s32(v_src_y1, v_height4));

                v_dst_index.val[0] = vbslq_s32(vandq_u32(v_mask_x0, v_mask_y0), v_dst_index.val[0], v_m1_4);
                v_dst_index.val[1] = vbslq_s32(vandq_u32(v_mask_x1, v_mask_y0), v_dst_index.val[1], v_m1_4);
                v_dst_index.val[2] = vbslq_s32(vandq_u32(v_mask_x0, v_mask_y1), v_dst_index.val[2], v_m1_4);
                v_dst_index.val[3] = vbslq_s32(vandq_u32(v_mask_x1, v_mask_y1), v_dst_index.val[3], v_m1_4);

                vst2q_f32(coeff_row + (x << 1), v_coeff);
                vst4q_s32(map_row + (x << 2), v_dst_index);
            }
        }
        vx_uint8 * dstBase = (vx_uint8 *)((vx_int8 *)dst_base + i * dstStride);
        remapLinearConst(blockHeight, blockWidth, src_base, &map[0], &coeffs[0], dstBase + j, dstStride, borderValue);
    }
}


static vx_bool read_pixel(void *base, vx_imagepatch_addressing_t *addr, vx_uint32 src_height, vx_uint32 src_width,
    vx_float32 x, vx_float32 y, vx_uint8 *pixel)
{
    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= src_width || y >= src_height);
    vx_uint32 bx, by;
    vx_uint8 *bpixel;

    if (out_of_bounds)
    {
        return vx_false_e;
    }

    // bounded x/y
    bx = x < 0 ? 0 : x >= src_width ? src_width - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= src_height ? src_height - 1 : (vx_uint32)y;

    vx_uint8 *new_ptr = NULL;
    vx_uint32 offset = (addr->stride_y * by + addr->stride_x * bx);
    new_ptr = (vx_uint8 *)base;
    bpixel = &new_ptr[offset];

    *pixel = *bpixel;

    return vx_true_e;
}

#define REMAP(low_y, high_y, low_x)                                                                                    \
    for (y = low_y; y < high_y; y++)                                                                                   \
    {                                                                                                                  \
        vx_uint8 *dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;                                                \
        for (x = low_x; x < high_x; x++)                                                                               \
        {                                                                                                              \
            vx_float32 src_x = 0.0f;                                                                                   \
            vx_float32 src_y = 0.0f;                                                                                   \
                                                                                                                       \
            vxGetRemapPoint(*table, x, y, &src_x, &src_y);                                                             \
                                                                                                                       \
            if (policy == VX_INTERPOLATION_NEAREST_NEIGHBOR)                                                           \
            {                                                                                                          \
                read_pixel(src_base, in->addr, in->image.height, in->image.width, src_x + 0.5f, src_y + 0.5f, dst);    \
                dst++;                                                                                                 \
            }                                                                                                          \
            else if (policy == VX_INTERPOLATION_BILINEAR)                                                              \
            {                                                                                                          \
                vx_uint8 tl = 0;                                                                                       \
                vx_uint8 tr = 0;                                                                                       \
                vx_uint8 bl = 0;                                                                                       \
                vx_uint8 br = 0;                                                                                       \
                vx_float32 xf = floorf(src_x);                                                                         \
                vx_float32 yf = floorf(src_y);                                                                         \
                vx_float32 dx = src_x - xf;                                                                            \
                vx_float32 dy = src_y - yf;                                                                            \
                vx_float32 a[] = { (1.0f - dx) * (1.0f - dy), (1.0f - dx) * (dy), (dx)* (1.0f - dy), (dx)* (dy), };    \
                vx_bool defined = vx_true_e;                                                                           \
                defined &= read_pixel(src_base, in->addr, in->image.height, in->image.width, xf + 0, yf + 0, &tl);     \
                defined &= read_pixel(src_base, in->addr, in->image.height, in->image.width, xf + 1, yf + 0, &tr);     \
                defined &= read_pixel(src_base, in->addr, in->image.height, in->image.width, xf + 0, yf + 1, &bl);     \
                defined &= read_pixel(src_base, in->addr, in->image.height, in->image.width, xf + 1, yf + 1, &br);     \
                if (defined)                                                                                           \
                    *dst = (vx_uint8)(a[0] * tl + a[2] * tr + a[1] * bl + a[3] * br);                                  \
                dst++;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    }


void Remap_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_remap *table = (vx_remap *)parameters[1];
    vx_scalar *stype = (vx_scalar *)parameters[2];
    vx_tile_t *out = (vx_tile_t *)parameters[3];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_uint8 *src_base = in->base[0] + in->tile_x;
    vx_uint8 *dst_base = out->base[0] + out->tile_x;

    vx_int32 policy = (vx_int32)*stype;

    if (low_y == 0 && low_x == 0)
    {
        REMAP(low_y, high_y, low_x)
    }
    else
    {
        REMAP(0, low_y, low_x)

        src_base = in->base[0];
        dst_base = out->base[0];
        REMAP(low_y, high_y, 0)
    }
}

static vx_status VX_CALLBACK vxRemapInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_remap table;
            vxQueryParameter(param, VX_PARAMETER_REF, &table, sizeof(table));
            if (table)
            {
                /* \todo what are we checking? */
                status = VX_SUCCESS;
                vxReleaseRemap(&table);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vx_enum policy = 0;
                    vxCopyScalar(scalar, &policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((policy == VX_INTERPOLATION_NEAREST_NEIGHBOR) ||
                        (policy == VX_INTERPOLATION_BILINEAR))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxRemapOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter tbl_param = vxGetParameterByIndex(node, 1);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if ((vxGetStatus((vx_reference)src_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)tbl_param) == VX_SUCCESS))
        {
            vx_image src = 0;
            vx_image dst = 0;
            vx_remap tbl = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            vxQueryParameter(tbl_param, VX_PARAMETER_REF, &tbl, sizeof(tbl));
            if ((src) && (dst) && (tbl))
            {
                vx_uint32 w1 = 0, h1 = 0;
                vx_uint32 w2 = 0, h2 = 0;
                vx_uint32 w3 = 0, h3 = 0;

                vxQueryImage(src, VX_IMAGE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
                vxQueryRemap(tbl, VX_REMAP_SOURCE_WIDTH, &w2, sizeof(w2));
                vxQueryRemap(tbl, VX_REMAP_SOURCE_HEIGHT, &h2, sizeof(h2));
                vxQueryRemap(tbl, VX_REMAP_DESTINATION_WIDTH, &w3, sizeof(w3));
                vxQueryRemap(tbl, VX_REMAP_DESTINATION_HEIGHT, &h3, sizeof(h3));

                if ((w1 == w2) && (h1 == h2))
                {
                    /* fill in the meta data with the attributes so that the checker will pass */
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = VX_DF_IMAGE_U8;
                    ptr->dim.image.width = w3;
                    ptr->dim.image.height = h3;
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&src);
                vxReleaseRemap(&tbl);
                vxReleaseImage(&dst);
            }
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&tbl_param);
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

vx_tiling_kernel_t remap_kernel =
{ 
    "org.khronos.openvx.tiling_remap",
    VX_KERNEL_REMAP_TILING,
    NULL,
    Remap_image_tiling_flexible,
    Remap_image_tiling_fast,
    4,
    { { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
      { VX_INPUT,  VX_TYPE_REMAP,  VX_PARAMETER_STATE_REQUIRED },
      { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL },
      { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED } },
    NULL,
    vxRemapInputValidator,
    vxRemapOutputValidator,
    NULL,
    NULL,
    { 16, 16 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};
