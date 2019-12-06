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
#include <vx_debug.h>

#include <VX/vx_types.h>
#include <VX/vx_lib_extras.h>

#include <stdlib.h>

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


// nodeless version of the Phase kernel
vx_status vxPhase(vx_image grad_x, vx_image grad_y, vx_image output)
{
    vx_uint32 x;
    vx_uint32 y;
    vx_df_image format = 0;
    vx_uint8* dst_base = NULL;
    void* src_base_x   = NULL;
    void* src_base_y   = NULL;
    vx_imagepatch_addressing_t src_addr_x = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t src_addr_y = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr   = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t rect;
    vx_status status = VX_FAILURE;

    if (grad_x == 0 && grad_y == 0)
        return VX_ERROR_INVALID_PARAMETERS;

    status  = VX_SUCCESS;

    status |= vxGetValidRegionImage(grad_x, &rect);

    vx_map_id map_id = 0;
    vx_map_id map_id1 = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(grad_x, &rect, 0, &map_id, &src_addr_x, &src_base_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(grad_y, &rect, 0, &map_id1, &src_addr_y, &src_base_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &result_map_id, &dst_addr, (void**)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    status |= vxQueryImage(grad_x, VX_IMAGE_FORMAT, &format, sizeof(format));

    size_t width = dst_addr.dim_x;
    size_t height = dst_addr.dim_y;

    FASTATAN2CONST(256.0f / 360.0f);
    size_t roiw16 = width >= 15 ? width - 15 : 0;
    size_t roiw8 = width >= 7 ? width - 7 : 0;

    float32x4_t v_05 = vdupq_n_f32(0.5f);

    if (format == VX_DF_IMAGE_S16)
    {
        for (size_t i = 0; i < height; ++i)
        {
            const vx_int16 * src0 = (vx_int16 *)src_base_x + i * src_addr_x.stride_y / 2;
            const vx_int16 * src1 = (vx_int16 *)src_base_y + i * src_addr_y.stride_y / 2;
            vx_uint8 * dst = (vx_uint8 *)dst_base + i * dst_addr.stride_y;

            size_t j = 0;

            for (; j < roiw16; j += 16)
            {
                int16x8_t v_src00 = vld1q_s16(src0 + j), v_src01 = vld1q_s16(src0 + j + 8);
                int16x8_t v_src10 = vld1q_s16(src1 + j), v_src11 = vld1q_s16(src1 + j + 8);

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

                vst1q_u8(dst + j, vcombine_u8(vmovn_u16(v_dst16s0),
                                              vmovn_u16(v_dst16s1)));
            }
            for (; j < roiw8; j += 8)
            {
                int16x8_t v_src0 = vld1q_s16(src0 + j);
                int16x8_t v_src1 = vld1q_s16(src1 + j);

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

                vst1_u8(dst + j, vmovn_u16(v_dst));
            }

            for (; j < width; j++)
            {
                vx_float32 x = src0[j], y = src1[j];
                vx_float32 a;
                FASTATAN2SCALAR(y, x, a);
                dst[j] = (vx_uint8)(vx_uint32)floor(a + 0.5f);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < height; ++i)
        {
            const vx_float32 * src0 = (vx_float32 *)src_base_x + i * src_addr_x.stride_y / 4;
            const vx_float32 * src1 = (vx_float32 *)src_base_y + i * src_addr_y.stride_y / 4;
            vx_uint8 * dst = (vx_uint8 *)dst_base + i * dst_addr.stride_y;

            size_t j = 0;

            for (; j < roiw8; j += 8)
            {
                float32x4_t v_src00 = vld1q_f32(src0 + j), v_src01 = vld1q_f32(src0 + j + 4);
                float32x4_t v_src10 = vld1q_f32(src1 + j), v_src11 = vld1q_f32(src1 + j + 4);

                float32x4_t v_dst32f0;
                
                // 0
                FASTATAN2VECTOR(v_src10, v_src00, v_dst32f0)
                // 1
                float32x4_t v_dst32f1;
                FASTATAN2VECTOR(v_src11, v_src01, v_dst32f1)

                uint16x8_t v_dst = vcombine_u16(vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f0, v_05))),
                                                vmovn_u32(vcvtq_u32_f32(vaddq_f32(v_dst32f1, v_05))));

                vst1_u8(dst + j, vmovn_u16(v_dst));
            }
            for (; j < width; j++)
            {
                vx_float32 a;
                FASTATAN2SCALAR(src1[j], src0[j], a)
                dst[j] = a;
            }
        }
    }

    status |= vxUnmapImagePatch(grad_x, map_id);
    status |= vxUnmapImagePatch(grad_y, map_id1);
    status |= vxUnmapImagePatch(output, result_map_id);

    return status;
}

