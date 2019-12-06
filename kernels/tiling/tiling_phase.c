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

#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */

static float32x4_t vrecpq_f32(float32x4_t val)
{
    float32x4_t reciprocal = vrecpeq_f32(val);
    reciprocal = vmulq_f32(vrecpsq_f32(val, reciprocal), reciprocal);
    reciprocal = vmulq_f32(vrecpsq_f32(val, reciprocal), reciprocal);
    return reciprocal;
}

#define FASTATAN2CONST(scale)                                                            \
        vx_float32 P1 = ((vx_float32)( 0.9997878412794807  * (180.0 / M_PI) * scale)),   \
        P3 = ((vx_float32)(-0.3258083974640975  * (180.0 / M_PI) * scale)),              \
        P5 = ((vx_float32)( 0.1555786518463281  * (180.0 / M_PI) * scale)),              \
        P7 = ((vx_float32)(-0.04432655554792128 * (180.0 / M_PI) * scale)),              \
         A_90 = ((vx_float32)(90.f * scale)),                                            \
        A_180 = ((vx_float32)(180.f * scale)),                                           \
        A_360 = ((vx_float32)(360.f * scale));                                           \
        float32x4_t eps = (vdupq_n_f32((vx_float32)DBL_EPSILON)),                        \
         _90 = (vdupq_n_f32(A_90)),                                                      \
        _180 = (vdupq_n_f32(A_180)),                                                     \
        _360 = (vdupq_n_f32(A_360)),                                                     \
           z = (vdupq_n_f32(0.0f)),                                                      \
        p1 = (vdupq_n_f32(P1)),                                                          \
        p3 = (vdupq_n_f32(P3)),                                                          \
        p5 = (vdupq_n_f32(P5)),                                                          \
        p7 = (vdupq_n_f32(P7));

#define FASTATAN2SCALAR(y, x, a)                                                    \
    {                                                                               \
        vx_float32 ax = abs(x), ay = abs(y);                                        \
        vx_float32 c, c2;                                                           \
        if (ax >= ay)                                                               \
        {                                                                           \
            c = ay / (ax + (vx_float32)DBL_EPSILON);                                \
            c2 = c * c;                                                             \
            a = (((P7 * c2 + P5) * c2 + P3) * c2 + P1) * c;                         \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            c = ax / (ay + (vx_float32)DBL_EPSILON);                                \
            c2 = c * c;                                                             \
            a = A_90 - (((P7 * c2 + P5) * c2 + P3) * c2 + P1) * c;                  \
        }                                                                           \
        if (x < 0)                                                                  \
            a = A_180 - a;                                                          \
        if (y < 0)                                                                  \
            a = A_360 - a;                                                          \
    }

#define FASTATAN2VECTOR(v_y, v_x, a)                                                \
    {                                                                               \
        float32x4_t ax = vabsq_f32(v_x), ay = vabsq_f32(v_y);                       \
        float32x4_t tmin = vminq_f32(ax, ay), tmax = vmaxq_f32(ax, ay);             \
        float32x4_t c = vmulq_f32(tmin, vrecpq_f32(vaddq_f32(tmax, eps)));          \
        float32x4_t c2 = vmulq_f32(c, c);                                           \
        a = vmulq_f32(c2, p7);                                                      \
                                                                                    \
        a = vmulq_f32(vaddq_f32(a, p5), c2);                                        \
        a = vmulq_f32(vaddq_f32(a, p3), c2);                                        \
        a = vmulq_f32(vaddq_f32(a, p1), c);                                         \
                                                                                    \
        a = vbslq_f32(vcgeq_f32(ax, ay), a, vsubq_f32(_90, a));                     \
        a = vbslq_f32(vcltq_f32(v_x, z), vsubq_f32(_180, a), a);                    \
        a = vbslq_f32(vcltq_f32(v_y, z), vsubq_f32(_360, a), a);                    \
                                                                                    \
    }


void Phase_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_ex_t *grad_x = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *grad_y = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];

    vx_uint8 *src_base_x = grad_x->base[0];
    vx_uint8 *src_base_y = grad_y->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = out->tile_x + out->tile_block.width;

    FASTATAN2CONST(256.0f / 360.0f);
    vx_uint32 roiw16 = high_x >= 15 ? high_x - 15 : 0;
    vx_uint32 roiw8 = high_x >= 7 ? high_x - 7 : 0;

    float32x4_t v_05 = vdupq_n_f32(0.5f);

    vx_df_image format = grad_x->image.format;

    if (format == VX_DF_IMAGE_S16)
    {
        for (y = low_y; y < high_y; y++)
        {
            const vx_int16 * src0 = (vx_int16 *)src_base_x + y * grad_x->addr->stride_y / 2;
            const vx_int16 * src1 = (vx_int16 *)src_base_y + y * grad_y->addr->stride_y / 2;
            vx_uint8 * dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;

            x = low_x;

            for (; x < roiw16; x += 16)
            {
                int16x8_t v_src00 = vld1q_s16(src0 + x), v_src01 = vld1q_s16(src0 + x + 8);
                int16x8_t v_src10 = vld1q_s16(src1 + x), v_src11 = vld1q_s16(src1 + x + 8);

                // 0
                float32x4_t v_src0_p = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src00)));
                float32x4_t v_src1_p = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src10)));
                float32x4_t v_dst32f0;
                FASTATAN2VECTOR(v_src1_p, v_src0_p, v_dst32f0);

                v_src0_p = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src00)));
                v_src1_p = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src10)));
                float32x4_t v_dst32f1;
                FASTATAN2VECTOR(v_src1_p, v_src0_p, v_dst32f1);

                uint16x8_t v_dst16s0 = vcombine_u16(vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f0, v_05))),
                                                    vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f1, v_05))));

                // 1
                v_src0_p = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src01)));
                v_src1_p = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src11)));
                FASTATAN2VECTOR(v_src1_p, v_src0_p, v_dst32f0);

                v_src0_p = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src01)));
                v_src1_p = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src11)));
                FASTATAN2VECTOR(v_src1_p, v_src0_p, v_dst32f1);

                uint16x8_t v_dst16s1 = vcombine_u16(vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f0, v_05))),
                                                    vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f1, v_05))));

                vst1q_u8(dst + x, vcombine_u8(vmovn_u16(v_dst16s0),
                                              vmovn_u16(v_dst16s1)));
            }

            for (; x < roiw8; x += 8)
            {
                int16x8_t v_src0 = vld1q_s16(src0 + x);
                int16x8_t v_src1 = vld1q_s16(src1 + x);

                float32x4_t v_src0_p = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src0)));
                float32x4_t v_src1_p = vcvtq_f32_s32(vmovl_s16(vget_low_s16(v_src1)));
                float32x4_t v_dst32f0;
                FASTATAN2VECTOR(v_src1_p, v_src0_p, v_dst32f0);

                v_src0_p = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src0)));
                v_src1_p = vcvtq_f32_s32(vmovl_s16(vget_high_s16(v_src1)));
                float32x4_t v_dst32f1;
                FASTATAN2VECTOR(v_src1_p, v_src0_p, v_dst32f1);

                uint16x8_t v_dst = vcombine_u16(vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f0, v_05))),
                                                vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f1, v_05))));

                vst1_u8(dst + x, vmovn_u16(v_dst));
            }

            for (; x < high_x; x++)
            {
                vx_float32 val_x = src0[x], val_y = src1[x];
                vx_float32 a;
                FASTATAN2SCALAR(val_y, val_x, a);
                dst[x] = (vx_uint8)(vx_uint32)floor(a + 0.5f);
            }
        }
    }
    else
    {
        for (y = low_y; y < high_y; y++)
        {
            const vx_float32 * src0 = (vx_float32 *)src_base_x + y * grad_x->addr->stride_y / 4;
            const vx_float32 * src1 = (vx_float32 *)src_base_y + y * grad_y->addr->stride_y / 4;
            vx_uint8 * dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;

            x = low_x;

            for (; x < roiw8; x += 8)
            {
                float32x4_t v_src00 = vld1q_f32(src0 + x), v_src01 = vld1q_f32(src0 + x + 4);
                float32x4_t v_src10 = vld1q_f32(src1 + x), v_src11 = vld1q_f32(src1 + x + 4);

                float32x4_t v_dst32f0;
                
                // 0
                FASTATAN2VECTOR(v_src10, v_src00, v_dst32f0)
                // 1
                float32x4_t v_dst32f1;
                FASTATAN2VECTOR(v_src11, v_src01, v_dst32f1)

                uint16x8_t v_dst = vcombine_u16(vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f0, v_05))),
                                                vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f1, v_05))));

                vst1_u8(dst + x, vmovn_u16(v_dst));
            }
            for (; x < high_x; x++)
            {
                vx_float32 val_x = src0[x], val_y = src1[x];
                vx_float32 a;
                FASTATAN2SCALAR(val_y, val_x, a);
                dst[x] = a;
            }
        }
    }
}


#define PHASE_S16(low_y, high_y, low_x)                                                     \
    for (y = low_y; y < high_y; y++)                                                        \
    {                                                                                       \
        const vx_int16 * src0 = (vx_int16 *)src_base_x + y * grad_x->addr->stride_y / 2;    \
        const vx_int16 * src1 = (vx_int16 *)src_base_y + y * grad_y->addr->stride_y / 2;    \
        vx_uint8 * dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;                    \
                                                                                            \
        for (x = low_x; x < high_x; x++)                                                    \
        {                                                                                   \
            vx_float32 val_x = src0[x], val_y = src1[x];                                    \
            vx_float32 a;                                                                   \
            FASTATAN2SCALAR(val_y, val_x, a);                                               \
            dst[x] = (vx_uint8)(vx_uint32)floor(a + 0.5f);                                  \
        }                                                                                   \
    }


#define PHASE_F32(low_y, high_y, low_x)                                                         \
    for (y = low_y; y < high_y; y++)                                                            \
    {                                                                                           \
        const vx_float32 * src0 = (vx_float32 *)src_base_x + y * grad_x->addr->stride_y / 4;    \
        const vx_float32 * src1 = (vx_float32 *)src_base_y + y * grad_y->addr->stride_y / 4;    \
        vx_uint8 * dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;                        \
                                                                                                \
        for (x = low_x; x < high_x; x++)                                                        \
        {                                                                                       \
            vx_float32 val_x = src0[x], val_y = src1[x];                                        \
            vx_float32 a;                                                                       \
            FASTATAN2SCALAR(val_y, val_x, a);                                                   \
            dst[x] = a;                                                                         \
        }                                                                                       \
    }


void Phase_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_ex_t *grad_x = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *grad_y = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];

    vx_uint8 *src_base_x = grad_x->base[0];
    vx_uint8 *src_base_y = grad_y->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    FASTATAN2CONST(256.0f / 360.0f);

    vx_df_image format = grad_x->image.format;

    if (format == VX_DF_IMAGE_S16)
    {
        if (low_y == 0 && low_x == 0)
        {
            PHASE_S16(low_y, high_y, low_x)
        }
        else
        {
            PHASE_S16(0, low_y, low_x)
            PHASE_S16(low_y, high_y, 0)
        }
    }
    else
    {
        if (low_y == 0 && low_x == 0)
        {
            PHASE_F32(low_y, high_y, low_x)
        }
        else
        {
            PHASE_F32(0, low_y, low_x)
            PHASE_F32(low_y, high_y, 0)
        }
    }
}
