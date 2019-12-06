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
#include <stdlib.h>
#include <venum.h>
#include <stdio.h>

vx_uint8 vx_lbp_s(vx_int16 x)
{
    if(x >= 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

vx_uint8 vx_lbp_u(vx_uint8 *g, vx_uint8 gc)
{
    vx_uint8 u1 = vx_lbp_s(g[7] - gc);
    vx_uint8 u2 = vx_lbp_s(g[0] - gc);

    vx_uint8 abs1 = abs(u1 - u2);

    vx_uint8 abs2 = 0;
    for(vx_int8 p = 1; p < 8; p++)
    {
        u1 = vx_lbp_s(g[p] - gc);
        u2 = vx_lbp_s(g[p-1] - gc);
        abs2 += abs(u1 - u2);
    }

    return abs1 + abs2;
}

vx_status vxLBPStandard(vx_image src, vx_int8 ksize, vx_image dst)
{
    vx_rectangle_t rect;
    vx_uint32 y = 0, x = 0;
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint8 gc, g[8], sum;

    status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id_src = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &map_id_dst, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    if( status != VX_SUCCESS )
    {
        return status;
    }

    if(ksize == 3)
    {
        uint8x16_t vPrv[3], vCur[3], vNxt[3];
        vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
        uint8x16_t vOne = vdupq_n_u8(1);
        for (y = 1; y < src_addr.dim_y - 1; y += src_addr.step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 1) * src_addr.stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (vx_uint8 idx = 0; idx < 3; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + 16 * src_addr.stride_x);
            }
            for (x = 0; x < w16; x += 16)
            {
                uint8x16_t vSum = vdupq_n_u8(0);
                uint8x16_t vTmp = vextq_u8(vPrv[0], vCur[0], 15);
                uint8x16_t vPred = vcgeq_u8(vTmp, vCur[1]);
                uint8x16_t vVal = vandq_u8(vPred, vOne);
                vSum = vVal;

                vPred = vcgeq_u8(vCur[0], vCur[1]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 1);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vCur[0], vNxt[0], 1);
                vPred = vcgeq_u8(vTmp, vCur[1]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 2);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vCur[1], vNxt[1], 1);
                vPred = vcgeq_u8(vTmp, vCur[1]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 3);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vCur[2], vNxt[2], 1);
                vPred = vcgeq_u8(vTmp, vCur[1]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 4);
                vSum = vaddq_u8(vVal, vSum);

                vPred = vcgeq_u8(vCur[2], vCur[1]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 5);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vPrv[2], vCur[2], 15);
                vPred = vcgeq_u8(vTmp, vCur[1]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 6);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vPrv[1], vCur[1], 15);
                vPred = vcgeq_u8(vTmp, vCur[1]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 7);
                vSum = vaddq_u8(vVal, vSum);

                vst1q_u8(ptr_dst + x, vSum);

                for (vx_uint8 idx = 0; idx < 3; idx++)
                {
                    vPrv[idx] = vCur[idx];
                    vCur[idx] = vNxt[idx];
                    vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + (x + 32) * src_addr.stride_x);
                }
            }
            x = w16;
            if (w16 < 1)
            {
                x = 1;
            }
            for (; x < src_addr.dim_x - 1; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-1,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+1,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                for(vx_int8 p = 0; p < 8; p++)
                {
                    sum += vx_lbp_s(g[p] - gc) * (1 << p);
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }
     else if (ksize == 5)
     {
        uint8x16_t vPrv[5], vCur[5], vNxt[5];
        vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
        uint8x16_t vOne = vdupq_n_u8(1);
        for (y = 2; y < src_addr.dim_y - 2; y += src_addr.step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 2) * src_addr.stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (vx_uint8 idx = 0; idx < 5; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + 16 * src_addr.stride_x);
            }
            for (x = 0; x < w16; x += 16)
            {
                uint8x16_t vSum = vdupq_n_u8(0);
                uint8x16_t vTmp = vextq_u8(vPrv[1], vCur[1], 15);
                uint8x16_t vPred = vcgeq_u8(vTmp, vCur[2]);
                uint8x16_t vVal = vandq_u8(vPred, vOne);
                vSum = vVal;

                vPred = vcgeq_u8(vCur[0], vCur[2]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 1);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vCur[1], vNxt[1], 1);
                vPred = vcgeq_u8(vTmp, vCur[2]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 2);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vCur[2], vNxt[2], 2);
                vPred = vcgeq_u8(vTmp, vCur[2]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 3);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vCur[3], vNxt[3], 1);
                vPred = vcgeq_u8(vTmp, vCur[2]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 4);
                vSum = vaddq_u8(vVal, vSum);

                vPred = vcgeq_u8(vCur[4], vCur[2]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 5);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vPrv[3], vCur[3], 15);
                vPred = vcgeq_u8(vTmp, vCur[2]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 6);
                vSum = vaddq_u8(vVal, vSum);

                vTmp = vextq_u8(vPrv[2], vCur[2], 14);
                vPred = vcgeq_u8(vTmp, vCur[2]);
                vVal = vandq_u8(vPred, vOne);
                vVal = vshlq_n_u8(vVal, 7);
                vSum = vaddq_u8(vVal, vSum);

                vst1q_u8(ptr_dst + x, vSum);

                for (vx_uint8 idx = 0; idx < 5; idx++)
                {
                    vPrv[idx] = vCur[idx];
                    vCur[idx] = vNxt[idx];
                    vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + (x + 32) * src_addr.stride_x);
                }
            }
            x = w16;
            if (w16 < 2)
            {
                x = 2;
            }
            for (; x < src_addr.dim_x - 2; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-2,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+2,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                for(vx_int8 p = 0; p < 8; p++)
                {
                    sum += vx_lbp_s(g[p] - gc) * (1 << p);
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }
     status |= vxUnmapImagePatch(src, map_id_src);
     status |= vxUnmapImagePatch(dst, map_id_dst);
     return status;
}

vx_status vxLBPModified(vx_image src, vx_image dst)
{
    vx_rectangle_t rect;
    vx_uint32 y = 0, x = 0;
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint8 avg, g[8], sum;
    uint8x16_t vPrv[3], vCur[3], vNxt[3], vG[8];
    vx_uint32 w16;
    uint8x16_t vOne = vdupq_n_u8(1);
    vx_uint8 szCoeff[8] = { 1 << 0, 1 << 1, 1 << 2, 1 << 3,
                           1 << 4, 1 << 5, 1 << 6, 1 << 7 };

    status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id_src = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &map_id_dst, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    if( status != VX_SUCCESS )
    {
        return status;
    }

    w16 = (src_addr.dim_x >> 4) << 4;
    for (y = 2; y < src_addr.dim_y - 2; y += src_addr.step_y)
    {
        vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 2) * src_addr.stride_y;
        vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
        for (vx_uint8 idx = 0, idxY = 0; idxY < 5; (idx++, idxY += 2))
        {
            vPrv[idx] = vdupq_n_u8(0);
            vCur[idx] = vld1q_u8(ptr_src + idxY * src_addr.stride_y);
            vNxt[idx] = vld1q_u8(ptr_src + idxY * src_addr.stride_y + 16 * src_addr.stride_x);
        }
        for (x = 0; x < w16; x += 16)
        {
            uint16x8_t vSumu16_lo = vdupq_n_u16(0);
            uint16x8_t vSumu16_hi = vdupq_n_u16(0);
            uint8x16_t vAvg, vPred, vSum;

            vG[0] = vextq_u8(vPrv[0], vCur[0], 14);
            vG[1] = vCur[0];
            vG[2] = vextq_u8(vCur[0], vNxt[0], 2);
            vG[3] = vextq_u8(vCur[1], vNxt[1], 2);
            vG[4] = vextq_u8(vCur[2], vNxt[2], 2);
            vG[5] = vCur[2];
            vG[6] = vextq_u8(vPrv[2], vCur[2], 14);
            vG[7] = vextq_u8(vPrv[1], vCur[1], 14);

            for (vx_uint8 idx = 0; idx < 8; idx++)
            {
                vSumu16_lo = vaddq_u16(vSumu16_lo, vmovl_u8(vget_low_u8(vG[idx])));
                vSumu16_hi = vaddq_u16(vSumu16_hi, vmovl_u8(vget_high_u8(vG[idx])));
            }

            vSumu16_lo = vaddq_u16(vSumu16_lo, vdupq_n_u16(1));
            vSumu16_hi = vaddq_u16(vSumu16_hi, vdupq_n_u16(1));
            vSumu16_lo = vshrq_n_u16(vSumu16_lo, 3);
            vSumu16_hi = vshrq_n_u16(vSumu16_hi, 3);
            vAvg = vcombine_u8(vmovn_u16(vSumu16_lo), vmovn_u16(vSumu16_hi));

            vSumu16_lo = vdupq_n_u16(0);
            vSumu16_hi = vdupq_n_u16(0);
            for (vx_uint8 idx = 0; idx < 8; idx++)
            {
                vPred = vcgtq_u8(vG[idx], vAvg);
                vPred = vandq_u8(vPred, vOne);
                vSumu16_lo = vmlaq_n_u16(vSumu16_lo, vmovl_u8(vget_low_u8(vPred)), szCoeff[idx]);
                vSumu16_hi = vmlaq_n_u16(vSumu16_hi, vmovl_u8(vget_high_u8(vPred)), szCoeff[idx]);
            }

            vSum = vcombine_u8(vmovn_u16(vSumu16_lo), vmovn_u16(vSumu16_hi));
            vst1q_u8(ptr_dst + x, vSum);

            for (vx_uint8 idx = 0, idxY = 0; idxY < 5; (idx++, idxY += 2))
            {
                vPrv[idx] = vCur[idx];
                vCur[idx] = vNxt[idx];
                vNxt[idx] = vld1q_u8(ptr_src + idxY * src_addr.stride_y + (x + 32) * src_addr.stride_x);
            }
        }
        x = w16;
        if (w16 < 2)
        {
            x = 2;
        }
        for (; x < src_addr.dim_x - 2; x += src_addr.step_x)
        {
            g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y-2,  &src_addr);
            g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-2,  &src_addr);
            g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y-2,  &src_addr);
            g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y,    &src_addr);
            g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y+2,  &src_addr);
            g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+2,  &src_addr);
            g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y+2,  &src_addr);
            g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y,    &src_addr);

            avg =(g[0] + g[1] + g[2] + g[3] + g[4] + g[5] + g[6] + g[7] + 1)/8;

            sum = 0;
            for(vx_int8 p = 0; p < 8; p++)
            {
                sum += ((g[p] > avg) * (1 << p));
            }

            vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            *dst_ptr = sum;
        }
     }
     status |= vxUnmapImagePatch(src, map_id_src);
     status |= vxUnmapImagePatch(dst, map_id_dst);
     return status;
}

vx_status vxLBPUniform(vx_image src, vx_int8 ksize, vx_image dst)
{
    vx_rectangle_t rect;
    vx_uint32 y = 0, x = 0;
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint8 gc, g[8], sum;
    vx_uint8 szCoeff[8] = { 1 << 0, 1 << 1, 1 << 2, 1 << 3,
                           1 << 4, 1 << 5, 1 << 6, 1 << 7 };
    uint8x16_t vOne = vdupq_n_u8(1);
    uint8x16_t vNine = vdupq_n_u8(9);
    uint8x16_t vTwo = vdupq_n_u8(2);

    status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id_src = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &map_id_dst, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    if( status != VX_SUCCESS )
    {
        return status;
    }

    if(ksize == 3)
    {
        uint8x16_t vPrv[3], vCur[3], vNxt[3], vG[8];
        vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
        for (y = 1; y < src_addr.dim_y - 1; y += src_addr.step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 1) * src_addr.stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (vx_uint8 idx = 0; idx < 3; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + 16 * src_addr.stride_x);
            }
            for (x = 0; x < w16; x += 16)
            {
                vG[0] = vextq_u8(vPrv[0], vCur[0], 15);
                vG[1] = vCur[0];
                vG[2] = vextq_u8(vCur[0], vNxt[0], 1);
                vG[3] = vextq_u8(vCur[1], vNxt[1], 1);
                vG[4] = vextq_u8(vCur[2], vNxt[2], 1);
                vG[5] = vCur[2];
                vG[6] = vextq_u8(vPrv[2], vCur[2], 15);
                vG[7] = vextq_u8(vPrv[1], vCur[1], 15);

                uint8x16_t vPred = vcgeq_u8(vG[7], vCur[1]);
                uint8x16_t vU1 = vandq_u8(vPred, vOne);
                vPred = vcgeq_u8(vG[0], vCur[1]);
                uint8x16_t vU2 = vandq_u8(vPred, vOne);
                uint8x16_t vAbs1 = vabdq_u8(vU1, vU2);
                uint8x16_t vAbs2 = vdupq_n_u8(0);

                for (vx_uint8 idx = 1; idx < 8; idx++)
                {
                    vPred = vcgeq_u8(vG[idx], vCur[1]);
                    vU1 = vandq_u8(vPred, vOne);
                    vPred = vcgeq_u8(vG[idx - 1], vCur[1]);
                    vU2 = vandq_u8(vPred, vOne);
                    vAbs2 = vaddq_u8(vAbs2, vabdq_u8(vU1, vU2));
                }
                vAbs1 = vaddq_u8(vAbs1, vAbs2);

                uint16x8_t vSumu16_lo = vdupq_n_u16(0);
                uint16x8_t vSumu16_hi = vdupq_n_u16(0);
                for (vx_uint8 idx = 0; idx < 8; idx++)
                {
                    vPred = vcgeq_u8(vG[idx], vCur[1]);
                    vPred = vandq_u8(vPred, vOne);
                    vSumu16_lo = vmlaq_n_u16(vSumu16_lo, vmovl_u8(vget_low_u8(vPred)), szCoeff[idx]);
                    vSumu16_hi = vmlaq_n_u16(vSumu16_hi, vmovl_u8(vget_high_u8(vPred)), szCoeff[idx]);
                }

                uint8x16_t vSum = vcombine_u8(vmovn_u16(vSumu16_lo), vmovn_u16(vSumu16_hi));
                vPred = vcleq_u8(vAbs1, vTwo);
                vSum = vbslq_u8(vPred, vSum, vNine);

                vst1q_u8(ptr_dst + x, vSum);

                for (vx_uint8 idx = 0; idx < 3; idx++)
                {
                    vPrv[idx] = vCur[idx];
                    vCur[idx] = vNxt[idx];
                    vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + (x + 32) * src_addr.stride_x);
                }
            }
            x = w16;
            if (w16 < 1)
            {
                x = 1;
            }
            for (; x < src_addr.dim_x - 1; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-1,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+1,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                if(vx_lbp_u(g, gc) <= 2)
                {
                    for (vx_uint8 p = 0; p < 8; p++)
                    {
                        sum += vx_lbp_s(g[p] - gc)*(1 << p);
                    }
                }
                else
                {
                    sum = 9;
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }
     else if (ksize == 5)
     {
        uint8x16_t vPrv[5], vCur[5], vNxt[5], vG[8];
        vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
        for (y = 2; y < src_addr.dim_y - 2; y += src_addr.step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 2) * src_addr.stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (vx_uint8 idx = 0; idx < 5; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + 16 * src_addr.stride_x);
            }
            for (x = 0; x < w16; x += 16)
            {
                vG[0] = vextq_u8(vPrv[1], vCur[1], 15);
                vG[1] = vCur[0];
                vG[2] = vextq_u8(vCur[1], vNxt[1], 1);
                vG[3] = vextq_u8(vCur[2], vNxt[2], 2);
                vG[4] = vextq_u8(vCur[3], vNxt[3], 1);
                vG[5] = vCur[4];
                vG[6] = vextq_u8(vPrv[3], vCur[3], 15);
                vG[7] = vextq_u8(vPrv[2], vCur[2], 14);

                uint8x16_t vPred = vcgeq_u8(vG[7], vCur[2]);
                uint8x16_t vU1 = vandq_u8(vPred, vOne);
                vPred = vcgeq_u8(vG[0], vCur[2]);
                uint8x16_t vU2 = vandq_u8(vPred, vOne);
                uint8x16_t vAbs1 = vabdq_u8(vU1, vU2);
                uint8x16_t vAbs2 = vdupq_n_u8(0);

                for (vx_uint8 idx = 1; idx < 8; idx++)
                {
                    vPred = vcgeq_u8(vG[idx], vCur[2]);
                    vU1 = vandq_u8(vPred, vOne);
                    vPred = vcgeq_u8(vG[idx - 1], vCur[2]);
                    vU2 = vandq_u8(vPred, vOne);
                    vAbs2 = vaddq_u8(vAbs2, vabdq_u8(vU1, vU2));
                }
                vAbs1 = vaddq_u8(vAbs1, vAbs2);

                uint16x8_t vSumu16_lo = vdupq_n_u16(0);
                uint16x8_t vSumu16_hi = vdupq_n_u16(0);
                for (vx_uint8 idx = 0; idx < 8; idx++)
                {
                    vPred = vcgeq_u8(vG[idx], vCur[2]);
                    vPred = vandq_u8(vPred, vOne);
                    vSumu16_lo = vmlaq_n_u16(vSumu16_lo, vmovl_u8(vget_low_u8(vPred)), szCoeff[idx]);
                    vSumu16_hi = vmlaq_n_u16(vSumu16_hi, vmovl_u8(vget_high_u8(vPred)), szCoeff[idx]);
                }

                uint8x16_t vSum = vcombine_u8(vmovn_u16(vSumu16_lo), vmovn_u16(vSumu16_hi));
                vPred = vcleq_u8(vAbs1, vTwo);
                vSum = vbslq_u8(vPred, vSum, vNine);

                vst1q_u8(ptr_dst + x, vSum);

                for (vx_uint8 idx = 0; idx < 5; idx++)
                {
                    vPrv[idx] = vCur[idx];
                    vCur[idx] = vNxt[idx];
                    vNxt[idx] = vld1q_u8(ptr_src + idx * src_addr.stride_y + (x + 32) * src_addr.stride_x);
                }
            }
            x = w16;
            if (w16 < 2)
            {
                x = 2;
            }
            for (; x < src_addr.dim_x - 2; x += src_addr.step_x)
            {
                g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y-1,  &src_addr);
                g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y-2,  &src_addr);
                g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y-1,  &src_addr);
                g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+2, y,    &src_addr);
                g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+1, y+1,  &src_addr);
                g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y+2,  &src_addr);
                g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-1, y+1,  &src_addr);
                g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x-2, y,    &src_addr);
                gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x,   y,    &src_addr);

                sum = 0;
                if(vx_lbp_u(g, gc) <= 2)
                {
                    for (vx_uint8 p = 0; p < 8; p++)
                    {
                        sum += vx_lbp_s(g[p] - gc)*(1 << p);
                    }
                }
                else
                {
                    sum = 9;
                }

                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = sum;
            }
         }
     }
     status |= vxUnmapImagePatch(src, map_id_src);
     status |= vxUnmapImagePatch(dst, map_id_dst);
     return status;
}

// nodeless version of the LBP kernel
vx_status vxLBP(vx_image src, vx_scalar sformat, vx_scalar ksize, vx_image dst)
{
    vx_status status = VX_FAILURE;
    vx_enum format;
    vx_int8 size;
    vxCopyScalar(sformat, &format, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyScalar(ksize, &size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    switch (format)
    {
        case VX_LBP:
            status = vxLBPStandard(src, size, dst);
            break;
        case VX_MLBP:
            status = vxLBPModified(src, dst);
            break;
        case VX_ULBP:
            status = vxLBPUniform(src, size, dst);
            break;
    }
    return status;
}

