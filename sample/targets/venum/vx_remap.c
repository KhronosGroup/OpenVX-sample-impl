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

/*!
 * \file
 * \brief The Remap Kernel
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include "vx_internal.h"
#include <arm_neon.h>

static vx_bool read_pixel(void* base, vx_imagepatch_addressing_t* addr,
    vx_float32 x, vx_float32 y, const vx_border_t* borders, vx_uint8* pixel)
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

    bpixel = vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = *bpixel;

    return vx_true_e;
}

static vx_param_description_t remap_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_REMAP,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
};

static void remapNearest(void *src, vx_imagepatch_addressing_t *src_addr, vx_remap table,
                         vx_border_t *borders, void *dst, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 y, x;
    vx_uint32 w8 = (dst_addr->dim_x >> 3) << 3;
    vx_uint8 *ptr_src = (vx_uint8 *)src;
    float32x4_t vOffset = vdupq_n_f32(0.5f);
    float32x4_t vZero = vdupq_n_f32(0.0f);
    float32x4_t vDimX = vdupq_n_f32((float)src_addr->dim_x);
    float32x4_t vDimY = vdupq_n_f32((float)src_addr->dim_y);

    for (y = 0; y < dst_addr->dim_y; y++)
    {
        float *ptr_map = (float *)ownFormatMemoryPtr(&table->memory, 0, 0, y, 0);
        vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
        for (x = 0; x < w8; x += 8)
        {
            float32x4_t tmp0_f32 = vld1q_f32(ptr_map);
            float32x4_t tmp1_f32 = vld1q_f32(ptr_map + 4);
            float32x4_t vX0_f32 = vdupq_n_f32(0.0f);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 0), vX0_f32, 0);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 2), vX0_f32, 1);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 0), vX0_f32, 2);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 2), vX0_f32, 3);
            float32x4_t vY0_f32 = vdupq_n_f32(0.0f);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 1), vY0_f32, 0);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 3), vY0_f32, 1);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 1), vY0_f32, 2);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 3), vY0_f32, 3);

            tmp0_f32 = vld1q_f32(ptr_map + 8);
            tmp1_f32 = vld1q_f32(ptr_map + 12);
            float32x4_t vX1_f32 = vdupq_n_f32(0.0f);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 0), vX1_f32, 0);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 2), vX1_f32, 1);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 0), vX1_f32, 2);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 2), vX1_f32, 3);
            float32x4_t vY1_f32 = vdupq_n_f32(0.0f);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 1), vY1_f32, 0);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 3), vY1_f32, 1);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 1), vY1_f32, 2);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 3), vY1_f32, 3);

            vX0_f32 = vaddq_f32(vX0_f32, vOffset);
            vY0_f32 = vaddq_f32(vY0_f32, vOffset);
            vX1_f32 = vaddq_f32(vX1_f32, vOffset);
            vY1_f32 = vaddq_f32(vY1_f32, vOffset);

            uint32x4_t vX0_pred = vcltq_f32(vX0_f32, vZero);
            vX0_f32 = vbslq_f32(vX0_pred, vZero, vX0_f32);
            uint32x4_t vY0_pred = vcltq_f32(vY0_f32, vZero);
            vY0_f32 = vbslq_f32(vY0_pred, vZero, vY0_f32);
            uint32x4_t tmp_pred = vcgeq_f32(vX0_f32, vDimX);
            vX0_f32 = vbslq_f32(tmp_pred, vDimX, vX0_f32);
            vX0_pred = vorrq_u32(vX0_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY0_f32, vDimY);
            vY0_f32 = vbslq_f32(tmp_pred, vDimY, vY0_f32);
            vY0_pred = vorrq_u32(vY0_pred, tmp_pred);
            vX0_pred = vorrq_u32(vX0_pred, vY0_pred);

            uint32x4_t vX1_pred = vcltq_f32(vX1_f32, vZero);
            vX1_f32 = vbslq_f32(vX1_pred, vZero, vX1_f32);
            uint32x4_t vY1_pred = vcltq_f32(vY1_f32, vZero);
            vY1_f32 = vbslq_f32(vY1_pred, vZero, vY1_f32);
            tmp_pred = vcgeq_f32(vX1_f32, vDimX);
            vX1_f32 = vbslq_f32(tmp_pred, vDimX, vX1_f32);
            vX1_pred = vorrq_u32(vX1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY1_f32, vDimY);
            vY1_f32 = vbslq_f32(tmp_pred, vDimY, vY1_f32);
            vY1_pred = vorrq_u32(vY1_pred, tmp_pred);
            vX1_pred = vorrq_u32(vX1_pred, vY1_pred);

            int32x4_t vX0_s32 = vcvtq_s32_f32(vX0_f32);
            int32x4_t vY0_s32 = vcvtq_s32_f32(vY0_f32);
            int32x4_t vX1_s32 = vcvtq_s32_f32(vX1_f32);
            int32x4_t vY1_s32 = vcvtq_s32_f32(vY1_f32);

            vX0_s32 = vmlaq_n_s32(vX0_s32, vY0_s32, (vx_int32)src_addr->stride_y);
            vX1_s32 = vmlaq_n_s32(vX1_s32, vY1_s32, (vx_int32)src_addr->stride_y);

            uint8x8_t vVal = vdup_n_u8(0);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX0_s32, 0)], vVal, 0);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX0_s32, 1)], vVal, 1);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX0_s32, 2)], vVal, 2);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX0_s32, 3)], vVal, 3);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX1_s32, 0)], vVal, 4);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX1_s32, 1)], vVal, 5);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX1_s32, 2)], vVal, 6);
            vVal = vset_lane_u8(ptr_src[vgetq_lane_s32(vX1_s32, 3)], vVal, 7);

            uint16x8_t predu16 = vcombine_u16(vmovn_u32(vX0_pred), vmovn_u32(vX1_pred));
            uint8x8_t predu8 = vmovn_u16(predu16);

            if (borders->mode == VX_BORDER_UNDEFINED)
            {
                uint8x8_t borderVal = vld1_u8(ptr_dst);
                vVal = vbsl_u8(predu8, borderVal, vVal);
            }
            else if (borders->mode == VX_BORDER_CONSTANT)
            {
                uint8x8_t borderVal = vdup_n_u8(borders->constant_value.U8);
                vVal = vbsl_u8(predu8, borderVal, vVal);
            }

            vst1_u8(ptr_dst, vVal);

            ptr_dst += 8;
            ptr_map += 16;
        }
        for (x = w8; x < dst_addr->dim_x; x++)
        {
            vx_float32 src_x = 0.0f;
            vx_float32 src_y = 0.0f;

            vx_uint8 *tmp_dst = vxFormatImagePatchAddress2d(dst, x, y, dst_addr);

            (void)vxGetRemapPoint(table, x, y, &src_x, &src_y);

            /* this rounds then truncates the decimal side */
            read_pixel(src, src_addr, src_x + 0.5f, src_y + 0.5f, borders, tmp_dst);
        }
    }
}

static void remapBilinear(void *src, vx_imagepatch_addressing_t *src_addr, vx_remap table,
                          vx_border_t *borders, void *dst, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 y, x;
    vx_uint32 w8 = (dst_addr->dim_x >> 3) << 3;
    vx_uint8 *ptr_src = (vx_uint8 *)src;
    float32x4_t vConst = vdupq_n_f32(1.0f);
    float32x4_t vZero = vdupq_n_f32(0.0f);
    float32x4_t vDimX = vdupq_n_f32((float)src_addr->dim_x);
    float32x4_t vDimY = vdupq_n_f32((float)src_addr->dim_y);
    float32x4_t borderVal = vdupq_n_f32(0.0f);
    if (borders->mode == VX_BORDER_CONSTANT)
    {
        borderVal = vdupq_n_f32((float)borders->constant_value.U8);
    }

    for (y = 0; y < dst_addr->dim_y; y++)
    {
        float *ptr_map = (float *)ownFormatMemoryPtr(&table->memory, 0, 0, y, 0);
        vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
        for (x = 0; x < w8; x += 8)
        {
            float32x4_t tmp0_f32 = vld1q_f32(ptr_map);
            float32x4_t tmp1_f32 = vld1q_f32(ptr_map + 4);
            float32x4_t vX0_f32 = vdupq_n_f32(0.0f);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 0), vX0_f32, 0);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 2), vX0_f32, 1);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 0), vX0_f32, 2);
            vX0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 2), vX0_f32, 3);
            float32x4_t vY0_f32 = vdupq_n_f32(0.0f);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 1), vY0_f32, 0);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 3), vY0_f32, 1);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 1), vY0_f32, 2);
            vY0_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 3), vY0_f32, 3);

            tmp0_f32 = vld1q_f32(ptr_map + 8);
            tmp1_f32 = vld1q_f32(ptr_map + 12);
            float32x4_t vX1_f32 = vdupq_n_f32(0.0f);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 0), vX1_f32, 0);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 2), vX1_f32, 1);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 0), vX1_f32, 2);
            vX1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 2), vX1_f32, 3);
            float32x4_t vY1_f32 = vdupq_n_f32(0.0f);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 1), vY1_f32, 0);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp0_f32, 3), vY1_f32, 1);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 1), vY1_f32, 2);
            vY1_f32 = vsetq_lane_f32(vgetq_lane_f32(tmp1_f32, 3), vY1_f32, 3);

            float32x4_t vrnX0_f32 = vcvtq_f32_s32(vcvtq_s32_f32(vX0_f32));
            float32x4_t vrnY0_f32 = vcvtq_f32_s32(vcvtq_s32_f32(vY0_f32));
            float32x4_t vrnX1_f32 = vcvtq_f32_s32(vcvtq_s32_f32(vX1_f32));
            float32x4_t vrnY1_f32 = vcvtq_f32_s32(vcvtq_s32_f32(vY1_f32));

            vX0_f32 = vsubq_f32(vX0_f32, vrnX0_f32);
            vY0_f32 = vsubq_f32(vY0_f32, vrnY0_f32);
            vX1_f32 = vsubq_f32(vX1_f32, vrnX1_f32);
            vY1_f32 = vsubq_f32(vY1_f32, vrnY1_f32);

            tmp0_f32 = vsubq_f32(vConst, vX0_f32);
            tmp1_f32 = vsubq_f32(vConst, vY0_f32);
            float32x4_t a00_f32 = vmulq_f32(tmp0_f32, tmp1_f32);
            float32x4_t a01_f32 = vmulq_f32(tmp0_f32, vY0_f32);
            float32x4_t a02_f32 = vmulq_f32(vX0_f32, tmp1_f32);
            float32x4_t a03_f32 = vmulq_f32(vX0_f32, vY0_f32);

            tmp0_f32 = vsubq_f32(vConst, vX1_f32);
            tmp1_f32 = vsubq_f32(vConst, vY1_f32);
            float32x4_t a10_f32 = vmulq_f32(tmp0_f32, tmp1_f32);
            float32x4_t a11_f32 = vmulq_f32(tmp0_f32, vY1_f32);
            float32x4_t a12_f32 = vmulq_f32(vX1_f32, tmp1_f32);
            float32x4_t a13_f32 = vmulq_f32(vX1_f32, vY1_f32);

            //calculate tl
            uint32x4_t tmp1_pred = vdupq_n_u32(0);
            vX0_f32 = vrnX0_f32;
            vY0_f32 = vrnY0_f32;
            uint32x4_t vlow_pred = vcltq_f32(vX0_f32, vZero);
            vX0_f32 = vbslq_f32(vlow_pred, vZero, vX0_f32);
            uint32x4_t tmp_pred = vcltq_f32(vY0_f32, vZero);
            vY0_f32 = vbslq_f32(tmp_pred, vZero, vY0_f32);
            vlow_pred = vorrq_u32(vlow_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX0_f32, vDimX);
            vX0_f32 = vbslq_f32(tmp_pred, vDimX, vX0_f32);
            vlow_pred = vorrq_u32(vlow_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY0_f32, vDimY);
            vY0_f32 = vbslq_f32(tmp_pred, vDimY, vY0_f32);
            vlow_pred = vorrq_u32(vlow_pred, tmp_pred);

            int32x4_t vX_s32 = vcvtq_s32_f32(vX0_f32);
            int32x4_t vY_s32 = vcvtq_s32_f32(vY0_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            float32x4_t tl_f32 = vdupq_n_f32(0);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), tl_f32, 0);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), tl_f32, 1);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), tl_f32, 2);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), tl_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                tl_f32 = vbslq_f32(vlow_pred, borderVal, tl_f32);
            }

            //calculate tr
            vX0_f32 = vaddq_f32(vrnX0_f32, vConst);
            vY0_f32 = vrnY0_f32;
            tmp_pred = vcltq_f32(vX0_f32, vZero);
            vX0_f32 = vbslq_f32(tmp_pred, vZero, vX0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcltq_f32(vY0_f32, vZero);
            vY0_f32 = vbslq_f32(tmp_pred, vZero, vY0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX0_f32, vDimX);
            vX0_f32 = vbslq_f32(tmp_pred, vDimX, vX0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY0_f32, vDimY);
            vY0_f32 = vbslq_f32(tmp_pred, vDimY, vY0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);

            vX_s32 = vcvtq_s32_f32(vX0_f32);
            vY_s32 = vcvtq_s32_f32(vY0_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            float32x4_t tr_f32 = vdupq_n_f32(0);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), tr_f32, 0);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), tr_f32, 1);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), tr_f32, 2);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), tr_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                tr_f32 = vbslq_f32(tmp1_pred, borderVal, tr_f32);
            }
            vlow_pred = vorrq_u32(vlow_pred, tmp1_pred);

            //calculate bl
            vX0_f32 = vrnX0_f32;
            vY0_f32 = vaddq_f32(vrnY0_f32, vConst);
            tmp1_pred = vdupq_n_u32(0);
            tmp_pred = vcltq_f32(vX0_f32, vZero);
            vX0_f32 = vbslq_f32(tmp_pred, vZero, vX0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcltq_f32(vY0_f32, vZero);
            vY0_f32 = vbslq_f32(tmp_pred, vZero, vY0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX0_f32, vDimX);
            vX0_f32 = vbslq_f32(tmp_pred, vDimX, vX0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY0_f32, vDimY);
            vY0_f32 = vbslq_f32(tmp_pred, vDimY, vY0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);

            vX_s32 = vcvtq_s32_f32(vX0_f32);
            vY_s32 = vcvtq_s32_f32(vY0_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            float32x4_t bl_f32 = vdupq_n_f32(0);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), bl_f32, 0);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), bl_f32, 1);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), bl_f32, 2);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), bl_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                bl_f32 = vbslq_f32(tmp1_pred, borderVal, bl_f32);
            }
            vlow_pred = vorrq_u32(vlow_pred, tmp1_pred);

            //calculate br
            vX0_f32 = vaddq_f32(vrnX0_f32, vConst);
            vY0_f32 = vaddq_f32(vrnY0_f32, vConst);
            tmp1_pred = vdupq_n_u32(0);
            tmp_pred = vcltq_f32(vX0_f32, vZero);
            vX0_f32 = vbslq_f32(tmp_pred, vZero, vX0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcltq_f32(vY0_f32, vZero);
            vY0_f32 = vbslq_f32(tmp_pred, vZero, vY0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX0_f32, vDimX);
            vX0_f32 = vbslq_f32(tmp_pred, vDimX, vX0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY0_f32, vDimY);
            vY0_f32 = vbslq_f32(tmp_pred, vDimY, vY0_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);

            vX_s32 = vcvtq_s32_f32(vX0_f32);
            vY_s32 = vcvtq_s32_f32(vY0_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            float32x4_t br_f32 = vdupq_n_f32(0);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), br_f32, 0);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), br_f32, 1);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), br_f32, 2);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), br_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                br_f32 = vbslq_f32(tmp1_pred, borderVal, br_f32);
            }
            vlow_pred = vorrq_u32(vlow_pred, tmp1_pred);

            //bilinear
            float32x4_t sum0_f32 = vmulq_f32(a00_f32, tl_f32);
            sum0_f32 = vmlaq_f32(sum0_f32, a02_f32, tr_f32);
            sum0_f32 = vmlaq_f32(sum0_f32, a01_f32, bl_f32);
            sum0_f32 = vmlaq_f32(sum0_f32, a03_f32, br_f32);

            //acquire other 4 values
            vX1_f32 = vrnX1_f32;
            vY1_f32 = vrnY1_f32;
            uint32x4_t vhigh_pred = vcltq_f32(vX1_f32, vZero);
            vX1_f32 = vbslq_f32(vhigh_pred, vZero, vX1_f32);
            tmp_pred = vcltq_f32(vY1_f32, vZero);
            vY1_f32 = vbslq_f32(tmp_pred, vZero, vY1_f32);
            vhigh_pred = vorrq_u32(vhigh_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX1_f32, vDimX);
            vX1_f32 = vbslq_f32(tmp_pred, vDimX, vX1_f32);
            vhigh_pred = vorrq_u32(vhigh_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY1_f32, vDimY);
            vY1_f32 = vbslq_f32(tmp_pred, vDimY, vY1_f32);
            vhigh_pred = vorrq_u32(vhigh_pred, tmp_pred);

            vX_s32 = vcvtq_s32_f32(vX1_f32);
            vY_s32 = vcvtq_s32_f32(vY1_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            tl_f32 = vdupq_n_f32(0);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), tl_f32, 0);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), tl_f32, 1);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), tl_f32, 2);
            tl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), tl_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                tl_f32 = vbslq_f32(vhigh_pred, borderVal, tl_f32);
            }

            vX1_f32 = vaddq_f32(vrnX1_f32, vConst);
            vY1_f32 = vrnY1_f32;
            tmp1_pred = vdupq_n_u32(0);
            tmp_pred = vcltq_f32(vX1_f32, vZero);
            vX1_f32 = vbslq_f32(tmp_pred, vZero, vX1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcltq_f32(vY1_f32, vZero);
            vY1_f32 = vbslq_f32(tmp_pred, vZero, vY1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX1_f32, vDimX);
            vX1_f32 = vbslq_f32(tmp_pred, vDimX, vX1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY1_f32, vDimY);
            vY1_f32 = vbslq_f32(tmp_pred, vDimY, vY1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);

            vX_s32 = vcvtq_s32_f32(vX1_f32);
            vY_s32 = vcvtq_s32_f32(vY1_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            tr_f32 = vdupq_n_f32(0);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), tr_f32, 0);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), tr_f32, 1);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), tr_f32, 2);
            tr_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), tr_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                tr_f32 = vbslq_f32(tmp1_pred, borderVal, tr_f32);
            }
            vhigh_pred = vorrq_u32(vhigh_pred, tmp1_pred);

            vX1_f32 = vrnX1_f32;
            vY1_f32 = vaddq_f32(vrnY1_f32, vConst);
            tmp1_pred = vdupq_n_u32(0);
            tmp_pred = vcltq_f32(vX1_f32, vZero);
            vX1_f32 = vbslq_f32(tmp_pred, vZero, vX1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcltq_f32(vY1_f32, vZero);
            vY1_f32 = vbslq_f32(tmp_pred, vZero, vY1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX1_f32, vDimX);
            vX1_f32 = vbslq_f32(tmp_pred, vDimX, vX1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY1_f32, vDimY);
            vY1_f32 = vbslq_f32(tmp_pred, vDimY, vY1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);

            vX_s32 = vcvtq_s32_f32(vX1_f32);
            vY_s32 = vcvtq_s32_f32(vY1_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            bl_f32 = vdupq_n_f32(0);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), bl_f32, 0);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), bl_f32, 1);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), bl_f32, 2);
            bl_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), bl_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                bl_f32 = vbslq_f32(tmp1_pred, borderVal, bl_f32);
            }
            vhigh_pred = vorrq_u32(vhigh_pred, tmp1_pred);

            vX1_f32 = vaddq_f32(vrnX1_f32, vConst);
            vY1_f32 = vaddq_f32(vrnY1_f32, vConst);
            tmp1_pred = vdupq_n_u32(0);
            tmp_pred = vcltq_f32(vX1_f32, vZero);
            vX1_f32 = vbslq_f32(tmp_pred, vZero, vX1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcltq_f32(vY1_f32, vZero);
            vY1_f32 = vbslq_f32(tmp_pred, vZero, vY1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vX1_f32, vDimX);
            vX1_f32 = vbslq_f32(tmp_pred, vDimX, vX1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);
            tmp_pred = vcgeq_f32(vY1_f32, vDimY);
            vY1_f32 = vbslq_f32(tmp_pred, vDimY, vY1_f32);
            tmp1_pred = vorrq_u32(tmp1_pred, tmp_pred);

            vX_s32 = vcvtq_s32_f32(vX1_f32);
            vY_s32 = vcvtq_s32_f32(vY1_f32);
            vX_s32 = vmlaq_n_s32(vX_s32, vY_s32, (vx_int32)src_addr->stride_y);
            br_f32 = vdupq_n_f32(0);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 0)]), br_f32, 0);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 1)]), br_f32, 1);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 2)]), br_f32, 2);
            br_f32 = vsetq_lane_f32((float)(ptr_src[vgetq_lane_s32(vX_s32, 3)]), br_f32, 3);
            if (borders->mode == VX_BORDER_CONSTANT)
            {
                br_f32 = vbslq_f32(tmp1_pred, borderVal, br_f32);
            }
            vhigh_pred = vorrq_u32(vhigh_pred, tmp1_pred);

            //bilinear
            float32x4_t sum1_f32 = vmulq_f32(a10_f32, tl_f32);
            sum1_f32 = vmlaq_f32(sum1_f32, a12_f32, tr_f32);
            sum1_f32 = vmlaq_f32(sum1_f32, a11_f32, bl_f32);
            sum1_f32 = vmlaq_f32(sum1_f32, a13_f32, br_f32);

            int32x4_t sum0_s32 = vcvtq_s32_f32(sum0_f32);
            int32x4_t sum1_s32 = vcvtq_s32_f32(sum1_f32);
            uint16x8_t sum_u16 = vcombine_u16(vreinterpret_u16_s16(vmovn_s32(sum0_s32)),
                vreinterpret_u16_s16(vmovn_s32(sum1_s32)));
            uint8x8_t sum_u8 = vmovn_u16(sum_u16);
            if (borders->mode == VX_BORDER_UNDEFINED)
            {
                uint16x8_t u16_pred = vcombine_u16(vmovn_u32(vlow_pred), vmovn_u32(vhigh_pred));
                uint8x8_t u8_pred = vmovn_u16(u16_pred);
                uint8x8_t vDstVal = vld1_u8(ptr_dst);
                sum_u8 = vbsl_u8(u8_pred, vDstVal, sum_u8);
            }

            vst1_u8(ptr_dst, sum_u8);

            ptr_dst += 8;
            ptr_map += 16;
        }
        for (x = w8; x < dst_addr->dim_x; x++)
        {
            vx_float32 src_x = 0.0f;
            vx_float32 src_y = 0.0f;

            vx_uint8 *tmp_dst = vxFormatImagePatchAddress2d(dst, x, y, dst_addr);

            (void)vxGetRemapPoint(table, x, y, &src_x, &src_y);

            vx_uint8 tl = 0;
            vx_uint8 tr = 0;
            vx_uint8 bl = 0;
            vx_uint8 br = 0;
            vx_float32 xf = floorf(src_x);
            vx_float32 yf = floorf(src_y);
            vx_float32 dx = src_x - xf;
            vx_float32 dy = src_y - yf;
            vx_float32 a[] =
            {
                (1.0f - dx) * (1.0f - dy),
                (1.0f - dx) * (dy),
                (dx)* (1.0f - dy),
                (dx)* (dy),
            };
            vx_bool defined = vx_true_e;
            defined &= read_pixel(src, src_addr, xf + 0, yf + 0, borders, &tl);
            defined &= read_pixel(src, src_addr, xf + 1, yf + 0, borders, &tr);
            defined &= read_pixel(src, src_addr, xf + 0, yf + 1, borders, &bl);
            defined &= read_pixel(src, src_addr, xf + 1, yf + 1, borders, &br);
            if (defined)
            {
                *tmp_dst = (vx_uint8)(a[0]*tl + a[2]*tr + a[1]*bl + a[3]*br);
            }
        }
    }
}

static vx_status VX_CALLBACK vxRemapKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;

    if (num == dimof(remap_kernel_params))
    {
        vx_image src_image = (vx_image)parameters[0];
        vx_remap table     = (vx_remap)parameters[1];
        vx_scalar stype    = (vx_scalar)parameters[2];
        vx_image dst_image = (vx_image)parameters[3];
        vx_enum policy     = 0;
        void *src_base     = NULL;
        void *dst_base     = NULL;
        vx_uint32 y        = 0u;
        vx_uint32 x        = 0u;
        vx_uint32 width    = 0u;
        vx_uint32 height   = 0u;
        vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
        vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
        vx_rectangle_t src_rect;
        vx_rectangle_t dst_rect;
        vx_border_t    borders;
        vx_map_id      src_map_id;
        vx_map_id      dst_map_id;

        vxCopyScalar(stype, &policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

        vxQueryImage(src_image, VX_IMAGE_WIDTH, &width, sizeof(width));
        vxQueryImage(src_image, VX_IMAGE_HEIGHT, &height, sizeof(height));

        src_rect.start_x = 0;
        src_rect.start_y = 0;
        src_rect.end_x   = width;
        src_rect.end_y   = height;

        vxQueryImage(dst_image, VX_IMAGE_WIDTH, &width, sizeof(width));
        vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &height, sizeof(height));

        dst_rect.start_x = 0;
        dst_rect.start_y = 0;
        dst_rect.end_x   = width;
        dst_rect.end_y   = height;

        vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

        status = VX_SUCCESS;

        status |= vxMapImagePatch(src_image, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        status |= vxMapImagePatch(dst_image, &dst_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

        if (status == VX_SUCCESS)
        {
            if (policy == VX_INTERPOLATION_NEAREST_NEIGHBOR)
            {
                remapNearest(src_base, &src_addr, table, &borders, dst_base, &dst_addr);
            }
            else if (policy == VX_INTERPOLATION_BILINEAR)
            {
                remapBilinear(src_base, &src_addr, table, &borders, dst_base, &dst_addr);
            }
        }

        status |= vxUnmapImagePatch(src_image, src_map_id);
        status |= vxUnmapImagePatch(dst_image, dst_map_id);
    }
    return status;
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

vx_kernel_description_t remap_kernel =
{
    VX_KERNEL_REMAP,
    "org.khronos.openvx.remap",
    vxRemapKernel,
    remap_kernel_params, dimof(remap_kernel_params),
    NULL,
    vxRemapInputValidator,
    vxRemapOutputValidator,
    NULL,
    NULL,
};
