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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <arm_neon.h>
#include <venum.h>
#include "tensor_utils.h"

static vx_status bilateralFilter_8u(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                                    vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                                    void* dst, vx_size* dst_strides)
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0, j = 0;
    vx_int32 radius = diameter/2;
    vx_size out_size = ComputeNumberOfElements(dims, num_of_dims);
    vx_float32 color_weight[256];
    vx_float32 space_weight[radius*2 + 1];
    vx_float32 gauss_color_coeff = -0.5/(sigma_color*sigma_color);
    vx_float32 gauss_space_coeff = -0.5/(sigma_space*sigma_space);

    if(radius < 1)
    {
        radius = 1;
    }

    if(radius > out_size)
    {
        radius = out_size;
    }

    for(i = 0; i < 256; i++)
    {
        color_weight[i] = (vx_float32)exp(i*i*gauss_color_coeff);
    }

    for(i = -radius; i <= radius; i++ )
    {
         space_weight[i+radius] = (vx_float32)exp(i*i*gauss_space_coeff);
    }

    vx_size src_pos = 0, dst_pos = 0, nei_pos = 0;
    vx_int8 *src_ptr = NULL, *dst_ptr = NULL, *nei_ptr = NULL;
    vx_float32 sum = 0, wsum = 0, w = 0;
    vx_int32 w8 = (((out_size - radius - radius) >> 3) << 3) + radius;
    src_ptr = (vx_int8 *)src;
    dst_ptr = (vx_int8 *)dst;

    for (i = radius; i < w8; i += 8)
    {
        int8x8_t vSrc = vld1_s8(src_ptr + i);

        float32x4_t vSum_lo = vdupq_n_f32(0);
        float32x4_t vSum_hi = vSum_lo;
        float32x4_t vWsum_lo = vSum_lo;
        float32x4_t vWsum_hi = vSum_lo;
        for(j = -radius; j <= radius; j++)
        {
            int8x8_t vNei = vld1_s8(src_ptr + i + j);
            int16x8_t vAbsIdx = vabdl_s8(vNei, vSrc);

            float32x4_t vSpaceW = vdupq_n_f32(space_weight[j+radius]);
            float32x4_t vColorW = vdupq_n_f32(0);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 0)], vColorW, 0);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 1)], vColorW, 1);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 2)], vColorW, 2);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 3)], vColorW, 3);

            float32x4_t vW = vmulq_f32(vSpaceW, vColorW);
            vSum_lo = vmlaq_f32(vSum_lo, vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vNei)))), vW);
            vWsum_lo = vaddq_f32(vWsum_lo, vW);

            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 4)], vColorW, 0);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 5)], vColorW, 1);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 6)], vColorW, 2);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s16(vAbsIdx, 7)], vColorW, 3);

            vW = vmulq_f32(vSpaceW, vColorW);
            vSum_hi = vmlaq_f32(vSum_hi, vcvtq_f32_s32(vmovl_s16(vget_high_s16(vmovl_s8(vNei)))), vW);
            vWsum_hi = vaddq_f32(vWsum_hi, vW);
        }

        vSum_lo = vmulq_f32(vSum_lo, vrecpeq_f32(vWsum_lo));
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 0)), vSum_lo, 0);
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 1)), vSum_lo, 1);
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 2)), vSum_lo, 2);
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 3)), vSum_lo, 3);

        vSum_hi = vmulq_f32(vSum_hi, vrecpeq_f32(vWsum_hi));
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 0)), vSum_hi, 0);
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 1)), vSum_hi, 1);
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 2)), vSum_hi, 2);
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 3)), vSum_hi, 3);

        int8x8_t vRet = vmovn_s16(vcombine_s16(vmovn_s32(vcvtq_s32_f32(vSum_lo)), vmovn_s32(vcvtq_s32_f32(vSum_hi))));
        vst1_s8(dst_ptr + i, vRet);
    }
    for (; i < (out_size - radius); i++)
    {
        src_pos = ComputeGlobalPositionsFromIndex(i, dims, src_strides, num_of_dims, &src_pos);
        dst_pos = ComputeGlobalPositionsFromIndex(i, dims, dst_strides, num_of_dims, &dst_pos);

        src_ptr = src+src_pos;
        dst_ptr = dst+dst_pos;

        sum = 0, wsum = 0;

        for(j = -radius; j <= radius; j++)
        {
            nei_pos = ComputeGlobalPositionsFromIndex(i+j, dims, src_strides, num_of_dims, &nei_pos);
            nei_ptr = src + nei_pos;
            w = space_weight[j+radius]*color_weight[abs(*nei_ptr - *src_ptr)];
            sum += (*nei_ptr)*w;
            wsum += w;
        }
        *dst_ptr = round(sum/wsum);
    }
    return status;
}

static vx_status bilateralFilter_16(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                                    vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                                    void* dst, vx_size* dst_strides)
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0, j = 0;
    vx_int32 radius = diameter/2;
    vx_size out_size = ComputeNumberOfElements(dims, num_of_dims);
    vx_float32 color_weight[256*256];
    vx_float32 space_weight[radius*2 + 1];
    vx_float32 gauss_color_coeff = -0.5/(sigma_color*sigma_color);
    vx_float32 gauss_space_coeff = -0.5/(sigma_space*sigma_space);

    if(radius < 1)
    {
        radius = 1;
    }

    if(radius > out_size)
    {
        radius = out_size;
    }

    for(i = 0; i < 256*256; i++)
    {
        color_weight[i] = (vx_float32)exp(i*i*gauss_color_coeff);
    }

    for(i = -radius; i <= radius; i++ )
    {
         space_weight[i+radius] = (vx_float32)exp(i*i*gauss_space_coeff);
    }

    vx_size src_pos = 0, dst_pos = 0, nei_pos = 0;
    vx_int16 *src_ptr = NULL, *dst_ptr = NULL, *nei_ptr = NULL;
    vx_float32 sum = 0, wsum = 0, w = 0;
    vx_int32 w8 = (((out_size - radius - radius) >> 3) << 3) + radius;
    src_ptr = (vx_int16 *)src;
    dst_ptr = (vx_int16 *)dst;

    for (i = radius; i < w8; i += 8)
    {
        int16x8_t vSrc = vld1q_s16(src_ptr + i);

        float32x4_t vSum_lo = vdupq_n_f32(0);
        float32x4_t vSum_hi = vSum_lo;
        float32x4_t vWsum_lo = vSum_lo;
        float32x4_t vWsum_hi = vSum_lo;
        for(j = -radius; j <= radius; j++)
        {
            int16x8_t vNei = vld1q_s16(src_ptr + i + j);

            int32x4_t vAbsIdx = vabdl_s16(vget_low_s16(vNei), vget_low_s16(vSrc));
            float32x4_t vSpaceW = vdupq_n_f32(space_weight[j+radius]);
            float32x4_t vColorW = vdupq_n_f32(0);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 0)], vColorW, 0);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 1)], vColorW, 1);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 2)], vColorW, 2);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 3)], vColorW, 3);

            float32x4_t vW = vmulq_f32(vSpaceW, vColorW);
            vSum_lo = vmlaq_f32(vSum_lo, vcvtq_f32_s32(vmovl_s16(vget_low_s16(vNei))), vW);
            vWsum_lo = vaddq_f32(vWsum_lo, vW);

            vAbsIdx = vabdl_s16(vget_high_s16(vNei), vget_high_s16(vSrc));
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 0)], vColorW, 0);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 1)], vColorW, 1);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 2)], vColorW, 2);
            vColorW = vsetq_lane_f32(color_weight[vgetq_lane_s32(vAbsIdx, 3)], vColorW, 3);

            vW = vmulq_f32(vSpaceW, vColorW);
            vSum_hi = vmlaq_f32(vSum_hi, vcvtq_f32_s32(vmovl_s16(vget_high_s16(vNei))), vW);
            vWsum_hi = vaddq_f32(vWsum_hi, vW);
        }
        float32x4_t vRecip = vrecpeq_f32(vWsum_lo);
        vRecip = vmulq_f32(vrecpsq_f32(vWsum_lo, vRecip), vRecip);
        vSum_lo = vmulq_f32(vSum_lo, vRecip);
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 0)), vSum_lo, 0);
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 1)), vSum_lo, 1);
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 2)), vSum_lo, 2);
        vSum_lo = vsetq_lane_f32(round(vgetq_lane_f32(vSum_lo, 3)), vSum_lo, 3);

        vRecip = vrecpeq_f32(vWsum_hi);
        vRecip = vmulq_f32(vrecpsq_f32(vWsum_hi, vRecip), vRecip);
        vSum_hi = vmulq_f32(vSum_hi, vRecip);
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 0)), vSum_hi, 0);
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 1)), vSum_hi, 1);
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 2)), vSum_hi, 2);
        vSum_hi = vsetq_lane_f32(round(vgetq_lane_f32(vSum_hi, 3)), vSum_hi, 3);
        int16x8_t vRet = vcombine_s16(vmovn_s32(vcvtq_s32_f32(vSum_lo)), vmovn_s32(vcvtq_s32_f32(vSum_hi)));
        vst1q_s16(dst_ptr + i, vRet);
    }

    for (; i < (out_size - radius); i++)
    {
        src_pos = ComputeGlobalPositionsFromIndex(i, dims, src_strides, num_of_dims, &src_pos);
        dst_pos = ComputeGlobalPositionsFromIndex(i, dims, dst_strides, num_of_dims, &dst_pos);

        src_ptr = src+src_pos;
        dst_ptr = dst+dst_pos;

        sum = 0, wsum = 0;

        for(j = -radius; j <= radius; j++)
        {
            nei_pos = ComputeGlobalPositionsFromIndex(i+j, dims, src_strides, num_of_dims, &nei_pos);
            nei_ptr = src + nei_pos;
            w = space_weight[j+radius]*color_weight[abs(*nei_ptr - *src_ptr)];
            sum += (*nei_ptr)*w;
            wsum += w;
        }
        *dst_ptr = round(sum/wsum);
    }
    return status;
}

vx_status vxBilateralFilter(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                            vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                            void* dst, vx_size* dst_strides, vx_enum type)
{
    vx_status status = VX_SUCCESS;

    if(type == VX_TYPE_UINT8)
    {
        status = bilateralFilter_8u(src, src_strides, dims, num_of_dims, diameter, sigma_space, sigma_color, dst, dst_strides);
    }
    else if(type == VX_TYPE_INT16)
    {
        status = bilateralFilter_16(src, src_strides, dims, num_of_dims, diameter, sigma_space, sigma_color, dst, dst_strides);
    }
    return status;
}
