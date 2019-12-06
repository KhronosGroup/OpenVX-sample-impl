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

#include <venum.h>
#include <arm_neon.h>

#define MIN(a,b)    (((a) < (b)) ? (a) : (b))

#define BLOCK_SIZE 32

static vx_bool read_pixel_1u_C1(void *base, vx_imagepatch_addressing_t *addr, vx_float32 x, vx_float32 y,
                                const vx_border_t *borders, vx_uint8 *pxl_group, vx_uint32 x_dst, vx_uint32 shift_x_u1)
{
    vx_bool out_of_bounds = (x < shift_x_u1 || y < 0 || x >= addr->dim_x || y >= addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 bpixel;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pxl_group = (*pxl_group & ~(1 << (x_dst % 8))) | ((borders->constant_value.U1 ? 1 : 0) << (x_dst % 8));
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < shift_x_u1 ? shift_x_u1 : x >= addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0          ? 0          : y >= addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (*(vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr) & (1 << (bx % 8))) >> (bx % 8);
    *pxl_group = (*pxl_group & ~(1 << (x_dst % 8))) | (bpixel << (x_dst % 8));

    return vx_true_e;
}

static vx_bool read_pixel_8u_C1(void *base, vx_imagepatch_addressing_t *addr,
                          vx_float32 x, vx_float32 y, const vx_border_t *borders, vx_uint8 *pixel)
{
    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= addr->dim_x || y >= addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 *bpixel;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pixel = borders->constant_value.U8;
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < 0 ? 0 : x >= addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = *bpixel;

    return vx_true_e;
}

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

// nodeless version of the WarpAffine kernel
vx_status vxWarpAffine_U8(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders)
{
    vx_status status = VX_SUCCESS;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_uint32 dst_width;
    vx_uint32 dst_height;
    vx_rectangle_t src_rect;
    vx_rectangle_t dst_rect;

    vx_float32 m[9];
    vx_enum type = 0;

    vx_uint32 x = 0u;
    vx_uint32 y = 0u;

    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;

    vxQueryImage(dst_image, VX_IMAGE_WIDTH, &dst_width, sizeof(dst_width));
    vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &dst_height, sizeof(dst_height));

    vxGetValidRegionImage(src_image, &src_rect);

    dst_rect.start_x = 0;
    dst_rect.start_y = 0;
    dst_rect.end_x   = dst_width;
    dst_rect.end_y   = dst_height;

    status |= vxMapImagePatch(src_image, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst_image, &dst_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    status |= vxCopyMatrix(matrix, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(stype, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    vx_uint32 src_width = src_addr.dim_x;
    vx_uint32 src_height = src_addr.dim_y;
    vx_uint32 srcStride = src_addr.stride_y;

    vx_uint32 dstStride = dst_addr.stride_y;

    int32x4_t v_width4 = vdupq_n_s32(src_width - 1), v_height4 = vdupq_n_s32(src_height - 1);
    int32x4_t v_step4 = vdupq_n_s32(srcStride);
    float32x4_t v_4 = vdupq_n_f32(4.0f);

    float32x4_t v_m0 = vdupq_n_f32(m[0]);
    float32x4_t v_m1 = vdupq_n_f32(m[1]);
    float32x4_t v_m2 = vdupq_n_f32(m[2]);
    float32x4_t v_m3 = vdupq_n_f32(m[3]);
    float32x4_t v_m4 = vdupq_n_f32(m[4]);
    float32x4_t v_m5 = vdupq_n_f32(m[5]);

    vx_uint8 borderValue = borders->constant_value.U8;

    if (status == VX_SUCCESS)
    {
        if (type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
        {
            vx_int32 _map[BLOCK_SIZE * BLOCK_SIZE + 16];
            vx_int32 * map = alignPtr(_map, 16);

            int32x4_t v_m1_4 = vdupq_n_s32(-1);
            float32x4_t v_zero4 = vdupq_n_f32(0.0f);

            for (size_t i = 0; i < dst_height; i += BLOCK_SIZE)
            {
                size_t blockHeight = MIN(BLOCK_SIZE, dst_height - i);
                for (size_t j = 0; j < dst_width; j += BLOCK_SIZE)
                {
                    size_t blockWidth = MIN(BLOCK_SIZE, dst_width - j);

                    // compute table
                    for (size_t y = 0; y < blockHeight; ++y)
                    {
                        vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(&map[0]) + y * blockWidth * sizeof(vx_int32));

                        size_t x = 0, y_ = y + i;
                        vx_float32 indeces[4] = { j + 0.0f, j + 1.0f, j + 2.0f, j + 3.0f };
                        float32x4_t v_x = vld1q_f32(indeces), v_y = vdupq_n_f32(y_);
                        float32x4_t v_yx = vmlaq_f32(v_m4, v_m2, v_y), v_yy = vmlaq_f32(v_m5, v_m3, v_y);

                        for ( ; x + 4 <= blockWidth; x += 4)
                        {
                            float32x4_t v_src_xf = vmlaq_f32(v_yx, v_m0, v_x);
                            float32x4_t v_src_yf = vmlaq_f32(v_yy, v_m1, v_x);

                            int32x4_t v_src_x = vcvtq_s32_f32(v_src_xf);
                            int32x4_t v_src_y = vcvtq_s32_f32(v_src_yf);
                            uint32x4_t v_mask = vandq_u32(vandq_u32(vcgeq_f32(v_src_xf, v_zero4), vcleq_s32(v_src_x, v_width4)),
                                                          vandq_u32(vcgeq_f32(v_src_yf, v_zero4), vcleq_s32(v_src_y, v_height4)));
                            int32x4_t v_src_index = vbslq_s32(v_mask, vmlaq_s32(v_src_x, v_src_y, v_step4), v_m1_4);
                            vst1q_s32(map_row + x, v_src_index);

                            v_x = vaddq_f32(v_x, v_4);
                        }

                        vx_float32 yx = m[2] * y_ + m[4], yy = m[3] * y_ + m[5];
                        for (ptrdiff_t x_ = x + j; x < blockWidth; ++x, ++x_)
                        {
                            vx_float32 src_x_f = m[0] * x_ + yx;
                            vx_float32 src_y_f = m[1] * x_ + yy;
                            vx_int32 src_x = floorf(src_x_f), src_y = floorf(src_y_f);

                            map_row[x] = (src_x >= 0) && (src_x < (vx_int32)src_width) &&
                                         (src_y >= 0) && (src_y < (vx_int32)src_height) ? src_y * srcStride + src_x : -1;
                        }
                    }
                
                    vx_uint8 * dstBase = (vx_uint8 *)((vx_int8 *)dst_base + i * dstStride);
                    // make remap
                    remapNearestNeighborConst(blockHeight, blockWidth, src_base, &map[0],
                                                        dstBase + j, dstStride, borderValue);
                }
            }
        }
        else if (type == VX_INTERPOLATION_BILINEAR)
        {
            vx_int32 _map[((BLOCK_SIZE * BLOCK_SIZE) << 2) + 16];
            vx_float32 _coeffs[((BLOCK_SIZE * BLOCK_SIZE) << 1) + 16];
            vx_int32 * map = alignPtr(_map, 16);
            vx_float32 * coeffs = alignPtr_f(_coeffs, 16);

            int32x4_t v_1 = vdupq_n_s32(1);
            float32x4_t v_zero4f = vdupq_n_f32(0.0f), v_one4f = vdupq_n_f32(1.0f);

            float32x4_t v_zero4 = vdupq_n_f32(0.0f);
            int32x4_t v_m1_4 = vdupq_n_s32(-1);

            for (size_t i = 0; i < dst_height; i += BLOCK_SIZE)
            {
                size_t blockHeight = MIN(BLOCK_SIZE, dst_height - i);
                for (size_t j = 0; j < dst_width; j += BLOCK_SIZE)
                {
                    size_t blockWidth = MIN(BLOCK_SIZE, dst_width - j);

                    // compute table
                    for (size_t y = 0; y < blockHeight; ++y)
                    {
                        vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(map) + y * blockWidth * sizeof(vx_int32) * 4);
                        vx_float32 * coeff_row = (vx_float32 *)((vx_int8 *)(coeffs) + y * blockWidth * sizeof(vx_float32) * 2);

                        size_t x = 0, y_ = y + i;
                        vx_float32 indeces[4] = { j + 0.0f, j + 1.0f, j + 2.0f, j + 3.0f };
                        float32x4_t v_x = vld1q_f32(indeces), v_y = vdupq_n_f32(y_), v_4 = vdupq_n_f32(4.0f);
                        float32x4_t v_yx = vmlaq_f32(v_m4, v_m2, v_y), v_yy = vmlaq_f32(v_m5, v_m3, v_y);

                        for ( ; x + 4 <= blockWidth; x += 4)
                        {
                            float32x4_t v_src_xf = vmlaq_f32(v_yx, v_m0, v_x);
                            float32x4_t v_src_yf = vmlaq_f32(v_yy, v_m1, v_x);

                            int32x4_t v_src_x0 = vcvtq_s32_f32(v_src_xf);
                            int32x4_t v_src_y0 = vcvtq_s32_f32(v_src_yf);

                            float32x4x2_t v_coeff;
                            v_coeff.val[0] = vsubq_f32(v_src_xf, vcvtq_f32_s32(v_src_x0));
                            v_coeff.val[1] = vsubq_f32(v_src_yf, vcvtq_f32_s32(v_src_y0));
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

                            uint32x4_t v_mask_x0 = vandq_u32(vcgeq_f32(v_src_xf, v_zero4), vcleq_s32(v_src_x0, v_width4));
                            uint32x4_t v_mask_x1 = vandq_u32(vcgeq_f32(vaddq_f32(v_src_xf, v_one4f), v_zero4), vcleq_s32(v_src_x1, v_width4));
                            uint32x4_t v_mask_y0 = vandq_u32(vcgeq_f32(v_src_yf, v_zero4), vcleq_s32(v_src_y0, v_height4));
                            uint32x4_t v_mask_y1 = vandq_u32(vcgeq_f32(vaddq_f32(v_src_yf, v_one4f), v_zero4), vcleq_s32(v_src_y1, v_height4));

                            v_dst_index.val[0] = vbslq_s32(vandq_u32(v_mask_x0, v_mask_y0), v_dst_index.val[0], v_m1_4);
                            v_dst_index.val[1] = vbslq_s32(vandq_u32(v_mask_x1, v_mask_y0), v_dst_index.val[1], v_m1_4);
                            v_dst_index.val[2] = vbslq_s32(vandq_u32(v_mask_x0, v_mask_y1), v_dst_index.val[2], v_m1_4);
                            v_dst_index.val[3] = vbslq_s32(vandq_u32(v_mask_x1, v_mask_y1), v_dst_index.val[3], v_m1_4);

                            vst2q_f32(coeff_row + (x << 1), v_coeff);
                            vst4q_s32(map_row + (x << 2), v_dst_index);

                            v_x = vaddq_f32(v_x, v_4);
                        }

                        vx_float32 yx = m[2] * y_ + m[4], yy = m[3] * y_ + m[5];
                        for (ptrdiff_t x_ = x + j; x < blockWidth; ++x, ++x_)
                        {
                            vx_float32 src_x_f = m[0] * x_ + yx;
                            vx_float32 src_y_f = m[1] * x_ + yy;

                            vx_int32 src0_x = (vx_int32)floorf(src_x_f), src1_x = src0_x + 1;
                            vx_int32 src0_y = (vx_int32)floorf(src_y_f), src1_y = src0_y + 1;

                            coeff_row[(x << 1) + 0] = src_x_f - src0_x;
                            coeff_row[(x << 1) + 1] = src_y_f - src0_y;

                            map_row[(x << 2) + 0] = (src0_x >= 0) && (src0_x < (vx_int32)src_width) &&
                                                    (src0_y >= 0) && (src0_y < (vx_int32)src_height) ? src0_y * srcStride + src0_x : -1;
                            map_row[(x << 2) + 1] = (src1_x >= 0) && (src1_x < (vx_int32)src_width) &&
                                                    (src0_y >= 0) && (src0_y < (vx_int32)src_height) ? src0_y * srcStride + src1_x : -1;
                            map_row[(x << 2) + 2] = (src0_x >= 0) && (src0_x < (vx_int32)src_width) &&
                                                    (src1_y >= 0) && (src1_y < (vx_int32)src_height) ? src1_y * srcStride + src0_x : -1;
                            map_row[(x << 2) + 3] = (src1_x >= 0) && (src1_x < (vx_int32)src_width) &&
                                                    (src1_y >= 0) && (src1_y < (vx_int32)src_height) ? src1_y * srcStride + src1_x : -1;
                        }
                    }

                    vx_uint8 * dstBase = (vx_uint8 *)((vx_int8 *)dst_base + i * dstStride);

                    remapLinearConst(blockHeight, blockWidth,
                                     src_base, &map[0], &coeffs[0],
                                     dstBase + j, dstStride, borderValue);
                }
            }
        }/*! \todo compute maximum area rectangle */
    }

    status |= vxCopyMatrix(matrix, m, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxUnmapImagePatch(src_image, src_map_id);
    status |= vxUnmapImagePatch(dst_image, dst_map_id);

    return status;
}

static inline float32x4_t vrecpq_f32(float32x4_t val)
{
    float32x4_t reciprocal = vrecpeq_f32(val);
    reciprocal = vmulq_f32(vrecpsq_f32(val, reciprocal), reciprocal);
    reciprocal = vmulq_f32(vrecpsq_f32(val, reciprocal), reciprocal);
    return reciprocal;
}

// nodeless version of the WarpPerspective kernel
vx_status vxWarpPerspective_U8(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders)
{
    vx_status status = VX_SUCCESS;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_uint32 dst_width;
    vx_uint32 dst_height;
    vx_rectangle_t src_rect;
    vx_rectangle_t dst_rect;

    vx_float32 m[9];
    vx_enum type = 0;

    vx_uint32 x = 0u;
    vx_uint32 y = 0u;

    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;

    vxQueryImage(dst_image, VX_IMAGE_WIDTH, &dst_width, sizeof(dst_width));
    vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &dst_height, sizeof(dst_height));

    vxGetValidRegionImage(src_image, &src_rect);

    dst_rect.start_x = 0;
    dst_rect.start_y = 0;
    dst_rect.end_x   = dst_width;
    dst_rect.end_y   = dst_height;

    status |= vxMapImagePatch(src_image, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst_image, &dst_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    status |= vxCopyMatrix(matrix, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(stype, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    vx_uint32 src_width = src_addr.dim_x;
    vx_uint32 src_height = src_addr.dim_y;
    vx_uint32 srcStride = src_addr.stride_y;

    vx_uint32 dstStride = dst_addr.stride_y;

    int32x4_t v_width4 = vdupq_n_s32(src_width - 1);
    int32x4_t v_height4 = vdupq_n_s32(src_height - 1);
    int32x4_t v_step4 = vdupq_n_s32(srcStride);
    float32x4_t v_4 = vdupq_n_f32(4.0f);

    float32x4_t v_m0 = vdupq_n_f32(m[0]);
    float32x4_t v_m1 = vdupq_n_f32(m[1]);
    float32x4_t v_m2 = vdupq_n_f32(m[2]);
    float32x4_t v_m3 = vdupq_n_f32(m[3]);
    float32x4_t v_m4 = vdupq_n_f32(m[4]);
    float32x4_t v_m5 = vdupq_n_f32(m[5]);
    float32x4_t v_m6 = vdupq_n_f32(m[6]);
    float32x4_t v_m7 = vdupq_n_f32(m[7]);
    float32x4_t v_m8 = vdupq_n_f32(m[8]);

    vx_uint8 borderValue = borders->constant_value.U8;

    if (type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
    {
        vx_int32 _map[BLOCK_SIZE * BLOCK_SIZE + 16];
        vx_int32 * map = alignPtr(_map, 16); 

        int32x4_t v_m1_4 = vdupq_n_s32(-1);
        float32x4_t v_zero4 = vdupq_n_f32(0.0f);

        for (size_t i = 0; i < dst_height; i += BLOCK_SIZE)
        {
            size_t blockHeight = MIN(BLOCK_SIZE, dst_height - i);
            for (size_t j = 0; j < dst_width; j += BLOCK_SIZE)
            {
                size_t blockWidth = MIN(BLOCK_SIZE, dst_width - j);

                // compute table
                for (size_t y = 0; y < blockHeight; ++y)
                {
                    vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(&map[0]) + y * blockWidth * sizeof(vx_int32));

                    size_t x = 0, y_ = y + i;
                    vx_float32  indeces[4] = { j + 0.0f, j + 1.0f, j + 2.0f, j + 3.0f };
                    float32x4_t v_x = vld1q_f32(indeces), v_y = vdupq_n_f32(y_);
                    float32x4_t v_yx = vmlaq_f32(v_m6, v_m3, v_y);
                    float32x4_t v_yy = vmlaq_f32(v_m7, v_m4, v_y);
                    float32x4_t v_yw = vmlaq_f32(v_m8, v_m5, v_y);

                    for ( ; x + 4 <= blockWidth; x += 4)
                    {
                        float32x4_t v_src_xf = vmlaq_f32(v_yx, v_m0, v_x);
                        float32x4_t v_src_yf = vmlaq_f32(v_yy, v_m1, v_x);

                        float32x4_t v_wf = vrecpq_f32(vmlaq_f32(v_yw, v_m2, v_x));

                        v_src_xf = vmulq_f32(v_wf, v_src_xf);
                        v_src_yf = vmulq_f32(v_wf, v_src_yf);

                        int32x4_t v_src_x = vcvtq_s32_f32(v_src_xf);
                        int32x4_t v_src_y = vcvtq_s32_f32(v_src_yf);
                        uint32x4_t v_mask = vandq_u32(vandq_u32(vcgeq_f32(v_src_xf, v_zero4), vcleq_s32(v_src_x, v_width4)),
                        vandq_u32(vcgeq_f32(v_src_yf, v_zero4), vcleq_s32(v_src_y, v_height4)));
                        int32x4_t v_src_index = vbslq_s32(v_mask, vmlaq_s32(v_src_x, v_src_y, v_step4), v_m1_4);
                        vst1q_s32(map_row + x, v_src_index);

                        v_x = vaddq_f32(v_x, v_4);
                    }

                    vx_float32 yx = m[3] * y_ + m[6], yy = m[4] * y_ + m[7], yw = m[5] * y_ + m[8];
                    for (ptrdiff_t x_ = x + j; x < blockWidth; ++x, ++x_)
                    {
                        vx_float32 w_f = 1.0f / (m[2] * x_ + yw);
                        vx_float32 src_x_f = (m[0] * x_ + yx) * w_f;
                        vx_float32 src_y_f = (m[1] * x_ + yy) * w_f;
                        vx_int32 src_x = floorf(src_x_f), src_y = floorf(src_y_f);

                        map_row[x] = (src_x >= 0) && (src_x < (vx_int32)src_width) &&
                        (src_y >= 0) && (src_y < (vx_int32)src_height) ? src_y * srcStride + src_x : -1;
                    }
                }

                vx_uint8 * dstBase = (vx_uint8 *)((vx_int8 *)dst_base + i * dstStride);
                // make remap
                remapNearestNeighborConst(blockHeight, blockWidth, src_base, &map[0],dstBase + j, dstStride, borderValue);
            }
        }
    }
    else if (type == VX_INTERPOLATION_BILINEAR)
    {
        vx_int32 _map[((BLOCK_SIZE * BLOCK_SIZE) << 2) + 16];
        vx_float32 _coeffs[((BLOCK_SIZE * BLOCK_SIZE) << 1) + 16];
        vx_int32 * map = alignPtr(_map, 16);
        vx_float32 * coeffs = alignPtr_f(_coeffs, 16);

        int32x4_t v_1 = vdupq_n_s32(1);
        float32x4_t v_zero4f = vdupq_n_f32(0.0f), v_one4f = vdupq_n_f32(1.0f);

        float32x4_t v_zero4 = vdupq_n_f32(0.0f);
        int32x4_t v_m1_4 = vdupq_n_s32(-1);

        for (size_t i = 0; i < dst_height; i += BLOCK_SIZE)
        {
            size_t blockHeight = MIN(BLOCK_SIZE, dst_height - i);
            for (size_t j = 0; j < dst_width; j += BLOCK_SIZE)
            {
                size_t blockWidth = MIN(BLOCK_SIZE, dst_width - j);

                // compute table
                for (size_t y = 0; y < blockHeight; ++y)
                {
                    vx_int32 * map_row = (vx_int32 *)((vx_int8 *)(map) + y * blockWidth * sizeof(vx_int32) * 4);
                    vx_float32 * coeff_row = (vx_float32 *)((vx_int8 *)(coeffs) + y * blockWidth * sizeof(vx_float32) * 2);

                    size_t x = 0, y_ = y + i;
                    vx_float32 indeces[4] = { j + 0.0f, j + 1.0f, j + 2.0f, j + 3.0f };
                    float32x4_t v_x = vld1q_f32(indeces), v_y = vdupq_n_f32(y_);
                    float32x4_t v_yx = vmlaq_f32(v_m6, v_m3, v_y), v_yy = vmlaq_f32(v_m7, v_m4, v_y),
                    v_yw = vmlaq_f32(v_m8, v_m5, v_y);

                    for ( ; x + 4 <= blockWidth; x += 4)
                    {
                        float32x4_t v_src_xf = vmlaq_f32(v_yx, v_m0, v_x);
                        float32x4_t v_src_yf = vmlaq_f32(v_yy, v_m1, v_x);
                        float32x4_t v_wf = vrecpq_f32(vmlaq_f32(v_yw, v_m2, v_x));
                        v_src_xf = vmulq_f32(v_wf, v_src_xf);
                        v_src_yf = vmulq_f32(v_wf, v_src_yf);

                        int32x4_t v_src_x0 = vcvtq_s32_f32(v_src_xf);
                        int32x4_t v_src_y0 = vcvtq_s32_f32(v_src_yf);

                        float32x4x2_t v_coeff;
                        v_coeff.val[0] = vsubq_f32(v_src_xf, vcvtq_f32_s32(v_src_x0));
                        v_coeff.val[1] = vsubq_f32(v_src_yf, vcvtq_f32_s32(v_src_y0));
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

                        uint32x4_t v_mask_x0 = vandq_u32(vcgeq_f32(v_src_xf, v_zero4), vcleq_s32(v_src_x0, v_width4));
                        uint32x4_t v_mask_x1 = vandq_u32(vcgeq_f32(vaddq_f32(v_src_xf, v_one4f), v_zero4), vcleq_s32(v_src_x1, v_width4));
                        uint32x4_t v_mask_y0 = vandq_u32(vcgeq_f32(v_src_yf, v_zero4), vcleq_s32(v_src_y0, v_height4));
                        uint32x4_t v_mask_y1 = vandq_u32(vcgeq_f32(vaddq_f32(v_src_yf, v_one4f), v_zero4), vcleq_s32(v_src_y1, v_height4));

                        v_dst_index.val[0] = vbslq_s32(vandq_u32(v_mask_x0, v_mask_y0), v_dst_index.val[0], v_m1_4);
                        v_dst_index.val[1] = vbslq_s32(vandq_u32(v_mask_x1, v_mask_y0), v_dst_index.val[1], v_m1_4);
                        v_dst_index.val[2] = vbslq_s32(vandq_u32(v_mask_x0, v_mask_y1), v_dst_index.val[2], v_m1_4);
                        v_dst_index.val[3] = vbslq_s32(vandq_u32(v_mask_x1, v_mask_y1), v_dst_index.val[3], v_m1_4);

                        vst2q_f32(coeff_row + (x << 1), v_coeff);
                        vst4q_s32(map_row + (x << 2), v_dst_index);

                        v_x = vaddq_f32(v_x, v_4);
                    }

                    vx_float32 yx = m[3] * y_ + m[6], yy = m[4] * y_ + m[7], yw = m[5] * y_ + m[8];
                    for (ptrdiff_t x_ = x + j; x < blockWidth; ++x, ++x_)
                    {
                        vx_float32 w_f = 1.0f / (m[2] * x_ + yw);
                        vx_float32 src_x_f = (m[0] * x_ + yx) * w_f;
                        vx_float32 src_y_f = (m[1] * x_ + yy) * w_f;

                        vx_int32 src0_x = (vx_int32)floorf(src_x_f), src1_x = src0_x + 1;
                        vx_int32 src0_y = (vx_int32)floorf(src_y_f), src1_y = src0_y + 1;

                        coeff_row[(x << 1) + 0] = src_x_f - src0_x;
                        coeff_row[(x << 1) + 1] = src_y_f - src0_y;

                        map_row[(x << 2) + 0] = (src0_x >= 0) && (src0_x < (vx_int32)src_width) &&
                                                (src0_y >= 0) && (src0_y < (vx_int32)src_height) ? src0_y * srcStride + src0_x : -1;
                        map_row[(x << 2) + 1] = (src1_x >= 0) && (src1_x < (vx_int32)src_width) &&
                                                (src0_y >= 0) && (src0_y < (vx_int32)src_height) ? src0_y * srcStride + src1_x : -1;
                        map_row[(x << 2) + 2] = (src0_x >= 0) && (src0_x < (vx_int32)src_width) &&
                                                (src1_y >= 0) && (src1_y < (vx_int32)src_height) ? src1_y * srcStride + src0_x : -1;
                        map_row[(x << 2) + 3] = (src1_x >= 0) && (src1_x < (vx_int32)src_width) &&
                                                (src1_y >= 0) && (src1_y < (vx_int32)src_height) ? src1_y * srcStride + src1_x : -1;
                    }
                }

                vx_uint8 * dstBase = (vx_uint8 *)((vx_int8 *)dst_base + i * dstStride);

                remapLinearConst(blockHeight, blockWidth,
                        src_base, &map[0], &coeffs[0],
                        dstBase + j, dstStride, borderValue);
            }
        }
    }/*! \todo compute maximum area rectangle */

    status |= vxCopyMatrix(matrix, m, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxUnmapImagePatch(src_image, src_map_id);
    status |= vxUnmapImagePatch(dst_image, dst_map_id);

    return status;
}

typedef void (*transform_f)(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y);

static void transform_affine(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y)
{
    *src_x = dst_x * m[0] + dst_y * m[2] + m[4];
    *src_y = dst_x * m[1] + dst_y * m[3] + m[5];
}

static void transform_perspective(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y)
{
    vx_float32 z = dst_x * m[2] + dst_y * m[5] + m[8];

    *src_x = (dst_x * m[0] + dst_y * m[3] + m[6]) / z;
    *src_y = (dst_x * m[1] + dst_y * m[4] + m[7]) / z;
}


static vx_status vxWarpGeneric(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image,
                                  const vx_border_t *borders, transform_f transform)
{
    vx_status status = VX_SUCCESS;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_uint32 dst_width;
    vx_uint32 dst_height;
    vx_rectangle_t src_rect;
    vx_rectangle_t dst_rect;
    vx_df_image format;

    vx_float32 m[9];
    vx_enum type = 0;

    vx_uint32 x = 0u;
    vx_uint32 y = 0u;

    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;

    status |= vxQueryImage(dst_image, VX_IMAGE_WIDTH, &dst_width, sizeof(dst_width));
    status |= vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &dst_height, sizeof(dst_height));
    status |= vxQueryImage(dst_image, VX_IMAGE_FORMAT, &format, sizeof(format));

    status |= vxGetValidRegionImage(src_image, &src_rect);
    vx_uint32 shift_x_u1 = src_rect.start_x % 8;  // Bit-shift offset for U1 images

    dst_rect.start_x = 0;
    dst_rect.start_y = 0;
    dst_rect.end_x   = dst_width;
    dst_rect.end_y   = dst_height;

    status |= vxMapImagePatch(src_image, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst_image, &dst_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    status |= vxCopyMatrix(matrix, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(stype, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if (status == VX_SUCCESS)
    {
        for (y = 0u; y < dst_addr.dim_y; y++)
        {
            for (x = 0u; x < dst_addr.dim_x; x++)
            {
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                vx_float32 xf;
                vx_float32 yf;
                transform(x, y, m, &xf, &yf);
                xf -= (vx_float32)src_rect.start_x;
                yf -= (vx_float32)src_rect.start_y;
                if (format == VX_DF_IMAGE_U1)   // Add bit-shift offset
                    xf += shift_x_u1;

                if (type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
                {
                    if (format == VX_DF_IMAGE_U1)
                        read_pixel_1u_C1(src_base, &src_addr, xf, yf, borders, dst, x, shift_x_u1);
                    else
                        read_pixel_8u_C1(src_base, &src_addr, xf, yf, borders, dst);
                }
                else if (type == VX_INTERPOLATION_BILINEAR)
                {
                    vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;
                    vx_bool defined = vx_true_e;
                    if (format == VX_DF_IMAGE_U1)
                    {
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf), floorf(yf), borders, &tl, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf), borders, &tr, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf), floorf(yf) + 1, borders, &bl, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf) + 1, borders, &br, 0, shift_x_u1);
                    }
                    else
                    {
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf), floorf(yf), borders, &tl);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf), borders, &tr);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf), floorf(yf) + 1, borders, &bl);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf) + 1, borders, &br);
                    }

                    if (defined)
                    {
                        vx_float32 ar = xf - floorf(xf);
                        vx_float32 ab = yf - floorf(yf);
                        vx_float32 al = 1.0f - ar;
                        vx_float32 at = 1.0f - ab;

                        if (format == VX_DF_IMAGE_U1)
                        {
                            // Arithmetic rounding instead of truncation for U1 images
                            vx_uint8 dst_val = (vx_uint8)(tl * al * at + tr * ar * at + bl * al * ab + br * ar * ab + 0.5);
                            *dst = (*dst & ~(1 << (x % 8))) | (dst_val << (x % 8));
                        }
                        else
                        {
                            *dst = (vx_uint8)(tl * al * at + tr * ar * at + bl * al * ab + br * ar * ab);
                        }
                    }
                }
            }
        }

        /*! \todo compute maximum area rectangle */
    }

    status |= vxCopyMatrix(matrix, m, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxUnmapImagePatch(src_image, src_map_id);
    status |= vxUnmapImagePatch(dst_image, dst_map_id);

    return status;
}



// nodeless version of the WarpAffine kernel
vx_status vxWarpAffine(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders)
{
    vx_df_image format;
    vx_rectangle_t src_rect;
    vx_status status = vxQueryImage(dst_image, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(src_image, &src_rect);
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1 || src_rect.start_x != 0)
        return vxWarpGeneric(src_image, matrix, stype, dst_image, borders, transform_affine);
    else
        return vxWarpAffine_U8(src_image, matrix, stype, dst_image, borders);
}

// nodeless version of the WarpPerspective kernel
vx_status vxWarpPerspective(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_t *borders)
{
    vx_df_image format;
    vx_rectangle_t src_rect;
    vx_status status = vxQueryImage(dst_image, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(src_image, &src_rect);
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1 || src_rect.start_x != 0)
        return vxWarpGeneric(src_image, matrix, stype, dst_image, borders, transform_perspective);
    else
        return vxWarpPerspective_U8(src_image, matrix, stype, dst_image, borders);
}
