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

static void addu8u8u8(void *src1, vx_imagepatch_addressing_t *src1_addr,
                      void *src2, vx_imagepatch_addressing_t *src2_addr,
                      vx_enum overflow_policy,
                      void *dst, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 y, x;
    vx_uint32 w16 = (src1_addr->dim_x >> 4) << 4;

    if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w16; x += 16)
            {
                uint8x16_t vSrc1 = vld1q_u8(ptr_src1 + x * src1_addr->stride_x);
                uint8x16_t vSrc2 = vld1q_u8(ptr_src2 + x * src2_addr->stride_x);
                int16x8_t vSrc1s16 = (int16x8_t)vmovl_u8(vget_low_u8(vSrc1));
                int16x8_t vSrc2s16 = (int16x8_t)vmovl_u8(vget_low_u8(vSrc2));
                int16x8_t vSum = vaddq_s16(vSrc1s16, vSrc2s16);
                vst1_u8(ptr_dst + x * dst_addr->stride_x, vqmovun_s16(vSum));

                vSrc1s16 = (int16x8_t)vmovl_u8(vget_high_u8(vSrc1));
                vSrc2s16 = (int16x8_t)vmovl_u8(vget_high_u8(vSrc2));
                vSum = vaddq_s16(vSrc1s16, vSrc2s16);
                vst1_u8(ptr_dst + x * dst_addr->stride_x + 8, vqmovun_s16(vSum));
            }
            for (x = w16; x < src1_addr->dim_x; x++)
            {
                vx_int16 val = (vx_int16)(*(ptr_src1 + x)) + (vx_int16)(*(ptr_src2) + x);
                *(ptr_dst + x) = val > UINT8_MAX ? UINT8_MAX : (vx_uint8)val;
            }
        }
    }
    else
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w16; x += 16)
            {
                uint8x16_t vSrc1 = vld1q_u8(ptr_src1 + x * src1_addr->stride_x);
                uint8x16_t vSrc2 = vld1q_u8(ptr_src2 + x * src2_addr->stride_x);
                int16x8_t vSrc1s16 = (int16x8_t)vmovl_u8(vget_low_u8(vSrc1));
                int16x8_t vSrc2s16 = (int16x8_t)vmovl_u8(vget_low_u8(vSrc2));
                int16x8_t vSum = vaddq_s16(vSrc1s16, vSrc2s16);
                vst1_u8(ptr_dst + x * dst_addr->stride_x, vmovn_u16((uint16x8_t)vSum));

                vSrc1s16 = (int16x8_t)vmovl_u8(vget_high_u8(vSrc1));
                vSrc2s16 = (int16x8_t)vmovl_u8(vget_high_u8(vSrc2));
                vSum = vaddq_s16(vSrc1s16, vSrc2s16);
                vst1_u8(ptr_dst + x * dst_addr->stride_x + 8, vmovn_u16((uint16x8_t)vSum));
            }
            for (x = w16; x < src1_addr->dim_x; x++)
            {
                vx_int16 val = (vx_int16)(*(ptr_src1 + x)) + (vx_int16)(*(ptr_src2) + x);
                *(ptr_dst + x) = (vx_uint8)val;
            }
        }
    }

    return;
}

static void addu8u8s16(void *src1, vx_imagepatch_addressing_t *src1_addr,
                       void *src2, vx_imagepatch_addressing_t *src2_addr,
                       vx_enum overflow_policy,
                       void *dst, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 y, x;
    vx_uint32 w16 = (src1_addr->dim_x >> 4) << 4;

    for (y = 0; y < src1_addr->dim_y; y++)
    {
        vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
        vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
        vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
        for (x = 0; x < w16; x += 16)
        {
            uint8x16_t vSrc1 = vld1q_u8(ptr_src1 + x * src1_addr->stride_x);
            uint8x16_t vSrc2 = vld1q_u8(ptr_src2 + x * src2_addr->stride_x);
            int16x8_t vSrc1s16 = (int16x8_t)vmovl_u8(vget_low_u8(vSrc1));
            int16x8_t vSrc2s16 = (int16x8_t)vmovl_u8(vget_low_u8(vSrc2));
            int16x8_t vSum = vaddq_s16(vSrc1s16, vSrc2s16);
            vst1q_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x), vSum);

            vSrc1s16 = (int16x8_t)vmovl_u8(vget_high_u8(vSrc1));
            vSrc2s16 = (int16x8_t)vmovl_u8(vget_high_u8(vSrc2));
            vSum = vaddq_s16(vSrc1s16, vSrc2s16);
            vst1q_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x + 16), vSum);
        }
        for (x = w16; x < src1_addr->dim_x; x++)
        {
            vx_int16 val = (vx_int16)(*(ptr_src1 + x)) + (vx_int16)(*(ptr_src2) + x);
            *(ptr_dst + x) = val;
        }
    }

    return;
}

static void addu8s16s16(void *src1, vx_imagepatch_addressing_t *src1_addr,
                        void *src2, vx_imagepatch_addressing_t *src2_addr,
                        vx_enum overflow_policy,
                        void *dst, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 y, x;
    vx_uint32 w8 = (src1_addr->dim_x >> 3) << 3;

    if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w8; x += 8)
            {
                uint8x8_t vSrc1 = vld1_u8(ptr_src1 + x * src1_addr->stride_x);
                int16x8_t vSrc2 = vld1q_s16((vx_int16 *)(ptr_src2 + x * src2_addr->stride_x));
                int16x8_t vSrc1s16 = (int16x8_t)vmovl_u8(vSrc1);

                int32x4_t vSrc1s32 = vmovl_s16(vget_low_s16(vSrc1s16));
                int32x4_t vSrc2s32 = vmovl_s16(vget_low_s16(vSrc2));
                int32x4_t vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x), vqmovn_s32(vSum));

                vSrc1s32 = vmovl_s16(vget_high_s16(vSrc1s16));
                vSrc2s32 = vmovl_s16(vget_high_s16(vSrc2));
                vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x + 8), vqmovn_s32(vSum));
            }
            for (x = w8; x < src1_addr->dim_x; x++)
            {
                int32_t val = (int32_t)(*(ptr_src1 + x)) + (int32_t)(*((vx_int16 *)ptr_src2 + x));
                *((vx_int16 *)ptr_dst + x) = val > INT16_MAX ? INT16_MAX : (val < INT16_MIN ? INT16_MIN : (vx_int16)val);
            }
        }
    }
    else
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w8; x += 8)
            {
                uint8x8_t vSrc1 = vld1_u8(ptr_src1 + x * src1_addr->stride_x);
                int16x8_t vSrc2 = vld1q_s16((vx_int16 *)(ptr_src2 + x * src2_addr->stride_x));
                int16x8_t vSrc1s16 = (int16x8_t)vmovl_u8(vSrc1);

                int32x4_t vSrc1s32 = vmovl_s16(vget_low_s16(vSrc1s16));
                int32x4_t vSrc2s32 = vmovl_s16(vget_low_s16(vSrc2));
                int32x4_t vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x), vmovn_s32(vSum));

                vSrc1s32 = vmovl_s16(vget_high_s16(vSrc1s16));
                vSrc2s32 = vmovl_s16(vget_high_s16(vSrc2));
                vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x + 8), vmovn_s32(vSum));
            }
            for (x = w8; x < src1_addr->dim_x; x++)
            {
                int32_t val = (int32_t)(*(ptr_src1 + x)) + (int32_t)(*((vx_int16 *)ptr_src2 + x));
                *((vx_int16 *)ptr_dst + x) = (vx_int16)val;
            }
        }
    }

    return;
}

static void adds16u8s16(void *src1, vx_imagepatch_addressing_t *src1_addr,
                        void *src2, vx_imagepatch_addressing_t *src2_addr,
                        vx_enum overflow_policy,
                        void *dst, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 y, x;
    vx_uint32 w8 = (src1_addr->dim_x >> 3) << 3;

    if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w8; x += 8)
            {
                int16x8_t vSrc1 = vld1q_s16((vx_int16 *)(ptr_src1 + x * src1_addr->stride_x));
                uint8x8_t vSrc2 = vld1_u8(ptr_src2 + x * src2_addr->stride_x);
                int16x8_t vSrc2s16 = (int16x8_t)vmovl_u8(vSrc2);

                int32x4_t vSrc1s32 = vmovl_s16(vget_low_s16(vSrc1));
                int32x4_t vSrc2s32 = vmovl_s16(vget_low_s16(vSrc2s16));
                int32x4_t vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x), vqmovn_s32(vSum));

                vSrc1s32 = vmovl_s16(vget_high_s16(vSrc1));
                vSrc2s32 = vmovl_s16(vget_high_s16(vSrc2s16));
                vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x + 8), vqmovn_s32(vSum));
            }
            for (x = w8; x < src1_addr->dim_x; x++)
            {
                int32_t val = (int32_t)(*((vx_int16 *)ptr_src1 + x)) + (int32_t)(*(ptr_src2 + x));
                *((vx_int16 *)ptr_dst + x) = val > INT16_MAX ? INT16_MAX : (val < INT16_MIN ? INT16_MIN : (vx_int16)val);
            }
        }
    }
    else
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w8; x += 8)
            {
                int16x8_t vSrc1 = vld1q_s16((vx_int16 *)(ptr_src1 + x * src1_addr->stride_x));
                uint8x8_t vSrc2 = vld1_u8(ptr_src2 + x * src2_addr->stride_x);
                int16x8_t vSrc2s16 = (int16x8_t)vmovl_u8(vSrc2);

                int32x4_t vSrc1s32 = vmovl_s16(vget_low_s16(vSrc1));
                int32x4_t vSrc2s32 = vmovl_s16(vget_low_s16(vSrc2s16));
                int32x4_t vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x), vmovn_s32(vSum));

                vSrc1s32 = vmovl_s16(vget_high_s16(vSrc1));
                vSrc2s32 = vmovl_s16(vget_high_s16(vSrc2s16));
                vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x + 8), vmovn_s32(vSum));
            }
            for (x = w8; x < src1_addr->dim_x; x++)
            {
                int32_t val = (int32_t)(*((vx_int16 *)ptr_src1 + x)) + (int32_t)(*(ptr_src2 + x));
                *((vx_int16 *)ptr_dst + x) = (vx_int16)val;
            }
        }
    }

    return;
}

static void adds16s16s16(void *src1, vx_imagepatch_addressing_t *src1_addr,
                         void *src2, vx_imagepatch_addressing_t *src2_addr,
                         vx_enum overflow_policy,
                         void *dst, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 y, x;
    vx_uint32 w8 = (src1_addr->dim_x >> 3) << 3;

    if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w8; x += 8)
            {
                int16x8_t vSrc1 = vld1q_s16((vx_int16 *)(ptr_src1 + x * src1_addr->stride_x));
                int16x8_t vSrc2 = vld1q_s16((vx_int16 *)(ptr_src2 + x * src2_addr->stride_x));

                int32x4_t vSrc1s32 = vmovl_s16(vget_low_s16(vSrc1));
                int32x4_t vSrc2s32 = vmovl_s16(vget_low_s16(vSrc2));
                int32x4_t vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x), vqmovn_s32(vSum));

                vSrc1s32 = vmovl_s16(vget_high_s16(vSrc1));
                vSrc2s32 = vmovl_s16(vget_high_s16(vSrc2));
                vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x + 8), vqmovn_s32(vSum));
            }
            for (x = w8; x < src1_addr->dim_x; x++)
            {
                int32_t val = (int32_t)(*((vx_int16 *)ptr_src1 + x)) + (int32_t)(*((vx_int16 *)ptr_src2 + x));
                *((vx_int16 *)ptr_dst + x) = val > INT16_MAX ? INT16_MAX : (val < INT16_MIN ? INT16_MIN : (vx_int16)val);
            }
        }
    }
    else
    {
        for (y = 0; y < src1_addr->dim_y; y++)
        {
            vx_uint8 *ptr_src1 = (vx_uint8 *)src1 + y * src1_addr->stride_y;
            vx_uint8 *ptr_src2 = (vx_uint8 *)src2 + y * src2_addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst + y * dst_addr->stride_y;
            for (x = 0; x < w8; x += 8)
            {
                int16x8_t vSrc1 = vld1q_s16((vx_int16 *)(ptr_src1 + x * src1_addr->stride_x));
                int16x8_t vSrc2 = vld1q_s16((vx_int16 *)(ptr_src2 + x * src2_addr->stride_x));

                int32x4_t vSrc1s32 = vmovl_s16(vget_low_s16(vSrc1));
                int32x4_t vSrc2s32 = vmovl_s16(vget_low_s16(vSrc2));
                int32x4_t vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x), vmovn_s32(vSum));

                vSrc1s32 = vmovl_s16(vget_high_s16(vSrc1));
                vSrc2s32 = vmovl_s16(vget_high_s16(vSrc2));
                vSum = vaddq_s32(vSrc1s32, vSrc2s32);
                vst1_s16((vx_int16 *)(ptr_dst + x * dst_addr->stride_x + 8), vmovn_s32(vSum));
            }
            for (x = w8; x < src1_addr->dim_x; x++)
            {
                int32_t val = (int32_t)(*((vx_int16 *)ptr_src1 + x)) + (int32_t)(*((vx_int16 *)ptr_src2 + x));
                *((vx_int16 *)ptr_dst + x) = (vx_int16)val;
            }
        }
    }

    return;
}


static void  vxSubSaturateu8u8u8(vx_uint32 width, vx_uint32 height, void *src0,void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;
    for (y = 0; y < height; y++)
    {
        vx_uint8* src0p = (vx_uint8 *)src0 + y * width;
        vx_uint8* src1p = (vx_uint8 *)src1 + y * width;
        vx_uint8* dstp = (vx_uint8 *)dst + y * width;

        for (x=0; x < roiw16; x+=step_16)
        {
            uint8x16_t v_src00 = vld1q_u8(src0p+x);
            uint8x16_t v_src10 = vld1q_u8(src1p+x);
            uint8x16_t v_dst;

            v_dst = vqsubq_u8(v_src00, v_src10);
            vst1q_u8(dstp+x, v_dst);
        }
        for (; x < width; x++)
        {
            vx_int16 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];
            if (final_result_value > UINT8_MAX)
                final_result_value = UINT8_MAX;

            if (final_result_value < 0)
                final_result_value = 0;

        dstp[x] = (vx_uint8)final_result_value;

        }
    }

}
static void  vxSubWrapu8u8u8(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_uint8* src0p = (vx_uint8 *)src0 + y * width;
        vx_uint8* src1p = (vx_uint8 *)src1 + y * width;
        vx_uint8* dstp = (vx_uint8 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {
            uint8x16_t v_src00 = vld1q_u8(src0p+x);
            uint8x16_t v_src10 = vld1q_u8(src1p+x);
            uint8x16_t v_dst;

            v_dst = vsubq_u8(v_src00, v_src10);
            vst1q_u8(dstp+x, v_dst);
        }
        for (; x < width; x++)
        {
            vx_int16 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];
            dstp[x] = (vx_uint8)final_result_value;

        }
    }
}
static void  vxSubSaturateu8s16s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_uint8* src0p = (vx_uint8 *)src0 + y * width;
        vx_int16* src1p = (vx_int16 *)src1 + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        for (x=0; x < roiw16; x+=step_16)
        {
            uint8x16_t v_src0 = vld1q_u8(src0p +x );
            int16x8_t v_src00 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v_src0)));
            int16x8_t v_src01 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(v_src0)));

            int16x8_t v_src10 = vld1q_s16(src1p + x);
            int16x8_t v_src11 = vld1q_s16(src1p + x + 8);

            int16x8_t v_dst0 = vqsubq_s16(v_src00, v_src10);
            int16x8_t v_dst1 = vqsubq_s16(v_src01, v_src11);
            vst1q_s16(dstp+x, v_dst0);
            vst1q_s16(dstp+x + 8, v_dst1);
        }
        for (; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];

            if (final_result_value > INT16_MAX)
                final_result_value = INT16_MAX;
            else if (final_result_value < INT16_MIN)
                final_result_value = INT16_MIN;

            dstp[x] = (vx_int16)final_result_value;

        }
   }
}
static void  vxSubWrapu8s16s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_uint8* src0p = (vx_uint8 *)src0 + y * width;
        vx_int16* src1p = (vx_int16 *)src1 + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {

            uint8x16_t v_src0 = vld1q_u8(src0p+x);
            int16x8_t v_src00 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v_src0)));
            int16x8_t v_src01 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(v_src0)));

            int16x8_t v_src10 = vld1q_s16(src1p+x);
            int16x8_t v_src11 = vld1q_s16(src1p+x + 8);

            int16x8_t v_dst0 = vsubq_s16(v_src00, v_src10);
            int16x8_t v_dst1 = vsubq_s16(v_src01, v_src11);
            vst1q_s16(dstp+x, v_dst0);
            vst1q_s16(dstp+x + 8, v_dst1);
        }
        for (; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];

            dstp[x] = (vx_int16)final_result_value;

        }
   }

}
static void  vxSubSaturateu8u8s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_uint8* src0p = (vx_uint8 *)src0 + y * width;
        vx_uint8* src1p = (vx_uint8 *)src1 + y * width;
        vx_uint16* dstu16 = (vx_uint16 *)dst + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {
            uint8x16_t v_src0 = vld1q_u8(src0p+x);
            uint8x16_t v_src1 = vld1q_u8(src1p+x);

            vst1q_u16(dstu16+x, vsubl_u8(vget_low_u8(v_src0), vget_low_u8(v_src1)));
            vst1q_u16(dstu16+x + 8, vsubl_u8(vget_high_u8(v_src0), vget_high_u8(v_src1)));

        }
        for (; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];

            if (final_result_value > INT16_MAX)
                final_result_value = INT16_MAX;
            else if (final_result_value < INT16_MIN)
                final_result_value = INT16_MIN;

            dstp[x] = (vx_int16)final_result_value;

        }
   }
}
static void  vxSubWrapu8u8s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_uint8* src0p = (vx_uint8 *)src0 + y * width;
        vx_uint8* src1p = (vx_uint8 *)src1 + y * width;
        vx_uint16* dstu16 = (vx_uint16 *)dst + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {
            uint8x16_t v_src0 = vld1q_u8(src0p+x);
            uint8x16_t v_src1 = vld1q_u8(src1p+x);

            vst1q_u16(dstu16+x, vsubl_u8(vget_low_u8(v_src0), vget_low_u8(v_src1)));
            vst1q_u16(dstu16+x + 8, vsubl_u8(vget_high_u8(v_src0), vget_high_u8(v_src1)));

        }
        for (; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];

            dstp[x] = (vx_int16)final_result_value;

        }
   }

}
static void  vxSubSaturates16u8s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_int16* src0p = (vx_int16 *)src0 + y * width;
        vx_uint8* src1p = (vx_uint8 *)src1 + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {
            int16x8_t v_src00 = vld1q_s16( src0p+x );
            int16x8_t v_src01 = vld1q_s16( src0p+x + 8 );

            uint8x16_t v_src1 = vld1q_u8(src1p+x);
            int16x8_t v_src10 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v_src1)));
            int16x8_t v_src11 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(v_src1)));

            vst1q_s16(dstp+x, vqsubq_s16(v_src00,v_src10));
            vst1q_s16(dstp+x + 8, vqsubq_s16(v_src01,v_src11));

        }
        for (x; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];

            if (final_result_value > INT16_MAX)
                final_result_value = INT16_MAX;
            else if (final_result_value < INT16_MIN)
                final_result_value = INT16_MIN;

            dstp[x] = (vx_int16)final_result_value;

        }
   }
}
static void  vxSubWraps16u8s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_int16* src0p = (vx_int16 *)src0 + y * width;
        vx_uint8* src1p = (vx_uint8 *)src1 + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {
            int16x8_t v_src00 = vld1q_s16( src0p+x );
            int16x8_t v_src01 = vld1q_s16( src0p+x + 8 );

            uint8x16_t v_src1 = vld1q_u8(src1p+x);
            int16x8_t v_src10 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v_src1)));
            int16x8_t v_src11 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(v_src1)));

            vst1q_s16(dstp+x, vsubq_s16(v_src00,v_src10));
            vst1q_s16(dstp+x + 8, vsubq_s16(v_src01,v_src11));
        }
        for (; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];
            dstp[x] = (vx_int16)final_result_value;
        }
   }

}
static void  vxSubSaturates16s16s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_int16* src0p = (vx_int16 *)src0 + y * width;
        vx_int16* src1p = (vx_int16 *)src1 + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {
            int16x8_t v_src00 = vld1q_s16( src0p+x );
            int16x8_t v_src01 = vld1q_s16( src0p+x + 8 );

            int16x8_t v_src10 = vld1q_s16( src1p+x );
            int16x8_t v_src11 = vld1q_s16( src1p+x + 8 );

            vst1q_s16(dstp+x, vqsubq_s16(v_src00,v_src10));
            vst1q_s16(dstp+x + 8, vqsubq_s16(v_src01,v_src11));
        }
        for (; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];

            if (final_result_value > INT16_MAX)
                final_result_value = INT16_MAX;
            else if (final_result_value < INT16_MIN)
                final_result_value = INT16_MIN;

            dstp[x] = (vx_int16)final_result_value;

        }
   }

}
static void  vxSubWraps16s16s16(vx_uint32 width, vx_uint32 height, void *src0, void *src1, void *dst)
{
    vx_uint32 y, x;

    vx_int32 step_16=16;
    vx_uint32 roiw16 = width >= step_16-1 ? width - (step_16-1) : 0;

    for (y = 0; y < height; y++)
    {
        vx_int16* src0p = (vx_int16 *)src0 + y * width;
        vx_int16* src1p = (vx_int16 *)src1 + y * width;
        vx_int16* dstp = (vx_int16 *)dst + y * width;

        x = 0;
        for (; x < roiw16; x+=step_16)
        {
            int16x8_t v_src00 = vld1q_s16( src0p+x );
            int16x8_t v_src01 = vld1q_s16( src0p+x + 8 );

            int16x8_t v_src10 = vld1q_s16( src1p+x );
            int16x8_t v_src11 = vld1q_s16( src1p+x + 8 );

            vst1q_s16(dstp+x, vsubq_s16(v_src00,v_src10));
            vst1q_s16(dstp+x + 8, vsubq_s16(v_src01,v_src11));
        }
        for (; x < width; x++)
        {
            vx_int32 final_result_value = (vx_int16)src0p[x] - (vx_int16)src1p[x];

            dstp[x] = (vx_int16)final_result_value;
        }
    }

}

// nodeless version of the Subtraction kernel
vx_status vxSubtraction(vx_image in0, vx_image in1, vx_scalar policy_param, vx_image output)
{
    vx_enum overflow_policy = -1;
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base[2] = { NULL, NULL };
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_df_image in0_format = 0;
    vx_df_image in1_format = 0;
    vx_df_image out_format = 0;
    vx_status status = VX_SUCCESS;

    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(in0, VX_IMAGE_FORMAT, &in0_format, sizeof(in0_format));
    vxQueryImage(in1, VX_IMAGE_FORMAT, &in1_format, sizeof(in1_format));

    status = vxGetValidRegionImage(in0, &rect);
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in0, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxCopyScalar(policy_param, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    width = src_addr[0].dim_x;
    height = src_addr[0].dim_y;
 
    if (in0_format == VX_DF_IMAGE_U8 && in1_format == VX_DF_IMAGE_U8 && out_format == VX_DF_IMAGE_U8)
    {
        /* Finally, overflow-check as per the target type and pol*/
        if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
        {
            vxSubSaturateu8u8u8(width, height, src_base[0], src_base[1],dst_base);
        }
        else
        {
            vxSubWrapu8u8u8(width, height, src_base[0], src_base[1], dst_base);
        }

    }
    else if (in0_format == VX_DF_IMAGE_U8 && in1_format == VX_DF_IMAGE_S16 && out_format == VX_DF_IMAGE_S16)
    {
        /* Finally, overflow-check as per the target type and pol*/
        if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
        {
            vxSubSaturateu8s16s16(width, height, src_base[0], src_base[1], dst_base);
        }
        else
        {
            vxSubWrapu8s16s16(width, height, src_base[0], src_base[1], dst_base);
        }
    }
    else if (in0_format == VX_DF_IMAGE_U8 && in1_format == VX_DF_IMAGE_U8 && out_format == VX_DF_IMAGE_S16)
    {
        /* Finally, overflow-check as per the target type and pol*/
        if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
        {
            vxSubSaturateu8u8s16(width, height, src_base[0], src_base[1], dst_base);
        }
        else
        {
            vxSubWrapu8u8s16(width, height, src_base[0], src_base[1], dst_base);
        }
    }
    else if (in0_format == VX_DF_IMAGE_S16 && in1_format == VX_DF_IMAGE_U8 && out_format == VX_DF_IMAGE_S16)
    {
        /* Finally, overflow-check as per the target type and pol*/
        if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
        {
            vxSubSaturates16u8s16(width, height, src_base[0], src_base[1], dst_base);
        }
        else
        {
            vxSubWraps16u8s16(width, height, src_base[0], src_base[1], dst_base);
        }
    }
    else if (in0_format == VX_DF_IMAGE_S16 && in1_format == VX_DF_IMAGE_S16 && out_format == VX_DF_IMAGE_S16)
    {
        /* Finally, overflow-check as per the target type and pol*/
        if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
        {
            vxSubSaturates16s16s16(width, height, src_base[0], src_base[1], dst_base);
        }
        else
        {
            vxSubWraps16s16s16(width, height, src_base[0], src_base[1], dst_base);
        }
    }

    status |= vxUnmapImagePatch(in0, map_id_0);
    status |= vxUnmapImagePatch(in1, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);

    return status;

}

// nodeless version of the Addition kernel
vx_status vxAddition(vx_image in0, vx_image in1, vx_scalar policy_param, vx_image output)
{
    vx_enum overflow_policy = -1;
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_df_image in0_format = 0;
    vx_df_image in1_format = 0;
    vx_df_image out_format = 0;
    vx_status status = VX_SUCCESS;

    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(in0, VX_IMAGE_FORMAT, &in0_format, sizeof(in0_format));
    vxQueryImage(in1, VX_IMAGE_FORMAT, &in1_format, sizeof(in1_format));

    status = vxGetValidRegionImage(in0, &rect);
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in0, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxCopyScalar(policy_param, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if (in0_format == VX_DF_IMAGE_U8)
    {
        if (in1_format == VX_DF_IMAGE_U8 && out_format == VX_DF_IMAGE_U8)
        {
            addu8u8u8(src_base[0], &(src_addr[0]), src_base[1], &(src_addr[1]),
                      overflow_policy, dst_base, &dst_addr);
        }
        else if (in1_format == VX_DF_IMAGE_U8 && out_format == VX_DF_IMAGE_S16)
        {
            addu8u8s16(src_base[0], &(src_addr[0]), src_base[1], &(src_addr[1]),
                       overflow_policy, dst_base, &dst_addr);
        }
        else if (in1_format == VX_DF_IMAGE_S16 && out_format == VX_DF_IMAGE_S16)
        {
            addu8s16s16(src_base[0], &(src_addr[0]), src_base[1], &(src_addr[1]),
                        overflow_policy, dst_base, &dst_addr);
        }
    }
    else if (in0_format == VX_DF_IMAGE_S16)
    {
        if (in1_format == VX_DF_IMAGE_U8 && out_format == VX_DF_IMAGE_S16)
        {
            adds16u8s16(src_base[0], &(src_addr[0]), src_base[1], &(src_addr[1]),
                        overflow_policy, dst_base, &dst_addr);
        }
        else if (in1_format == VX_DF_IMAGE_S16 && out_format == VX_DF_IMAGE_S16)
        {
            adds16s16s16(src_base[0], &(src_addr[0]), src_base[1], &(src_addr[1]),
                         overflow_policy, dst_base, &dst_addr);
        }
    }

    status |= vxUnmapImagePatch(in0, map_id_0);
    status |= vxUnmapImagePatch(in1, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);

    return status;
}