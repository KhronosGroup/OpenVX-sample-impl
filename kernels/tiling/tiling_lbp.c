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

static void vxLBPStandard_tiling_fast(vx_tile_ex_t *in, vx_int8 ksize, vx_tile_ex_t *out)
{
    vx_uint32 x = 0, y = 0;

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = in->tile_y + in->tile_block.height;

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = in->tile_x + in->tile_block.width;

    if(ksize == 3)
    {
        if (low_y == 0)
        {
            low_y = 1;
        }
        if (high_y == in->image.height)
        {
            high_y = high_y - 1;
        }
        if (high_x == in->image.width)
        {
            high_x = high_x - 1;
        }

        uint8x16_t vPrv[3], vCur[3], vNxt[3];
        uint8x16_t vOne = vdupq_n_u8(1);

        for (y = low_y; y < high_y; y += in->addr->step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 1) * in->addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;
            for (vx_uint8 idx = 0; idx < 3; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + 16 * in->addr->stride_x);
            }
            for (x = 0; x < high_x; x += 16)
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
                    vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + (x + 32) * in->addr->stride_x);
                }
            }
         }
     }
     else if (ksize == 5)
     {
        if (low_y == 0)
        {
            low_y = 2;
        }
        if (high_y == in->image.height)
        {
            high_y = high_y - 2;
        }
        if (high_x == in->image.width)
        {
            high_x = high_x - 2;
        }

        uint8x16_t vPrv[5], vCur[5], vNxt[5];
        uint8x16_t vOne = vdupq_n_u8(1);

        for (y = low_y; y < high_y; y += in->addr->step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 2) * in->addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;
            for (vx_uint8 idx = 0; idx < 5; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + 16 * in->addr->stride_x);
            }
            for (x = 0; x < high_x; x += 16)
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
                    vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + (x + 32) * in->addr->stride_x);
                }
            }
         }
     }
}

static void vxLBPModified_tiling_fast(vx_tile_ex_t *in, vx_tile_ex_t *out)
{
    vx_uint32 x = 0, y = 0;

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = in->tile_y + in->tile_block.height;

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = in->tile_x + in->tile_block.width;

    uint8x16_t vPrv[3], vCur[3], vNxt[3], vG[8];
    vx_uint32 w16;
    uint8x16_t vOne = vdupq_n_u8(1);
    vx_uint8 szCoeff[8] = { 1 << 0, 1 << 1, 1 << 2, 1 << 3,
                           1 << 4, 1 << 5, 1 << 6, 1 << 7 };

    if (low_y == 0)
    {
        low_y = 2;
    }
    if (high_y == in->image.height)
    {
        high_y = high_y - 2;
    }
    if (high_x == in->image.width)
    {
        high_x = high_x - 2;
    }

    for (y = low_y; y < high_y; y += in->addr->step_y)
    {
        vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 2) * in->addr->stride_y;
        vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;
        for (vx_uint8 idx = 0, idxY = 0; idxY < 5; (idx++, idxY += 2))
        {
            vPrv[idx] = vdupq_n_u8(0);
            vCur[idx] = vld1q_u8(ptr_src + idxY * in->addr->stride_y);
            vNxt[idx] = vld1q_u8(ptr_src + idxY * in->addr->stride_y + 16 * in->addr->stride_x);
        }
        for (x = 0; x < high_x; x += 16)
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
                vNxt[idx] = vld1q_u8(ptr_src + idxY * in->addr->stride_y + (x + 32) * in->addr->stride_x);
            }
        }
     }
}

static void vxLBPUniform_tiling_fast(vx_tile_ex_t *in, vx_int8 ksize, vx_tile_ex_t *out)
{
    vx_uint32 x = 0, y = 0;

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = in->tile_y + in->tile_block.height;

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = in->tile_x + in->tile_block.width;

    vx_uint8 szCoeff[8] = { 1 << 0, 1 << 1, 1 << 2, 1 << 3,
                           1 << 4, 1 << 5, 1 << 6, 1 << 7 };

    uint8x16_t vOne = vdupq_n_u8(1);
    uint8x16_t vNine = vdupq_n_u8(9);
    uint8x16_t vTwo = vdupq_n_u8(2);

    if(ksize == 3)
    {
        if (low_y == 0)
        {
            low_y = 1;
        }
        if (high_y == in->image.height)
        {
            high_y = high_y - 1;
        }
        if (high_x == in->image.width)
        {
            high_x = high_x - 1;
        }

        uint8x16_t vPrv[3], vCur[3], vNxt[3], vG[8];

        for (y = low_y; y < high_y; y += in->addr->step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 1) * in->addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;
            for (vx_uint8 idx = 0; idx < 3; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + 16 * in->addr->stride_x);
            }
            for (x = 0; x < high_x; x += 16)
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
                    vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + (x + 32) * in->addr->stride_x);
                }
            }
         }
     }
     else if (ksize == 5)
     {
        if (low_y == 0)
        {
            low_y = 2;
        }
        if (high_y == in->image.height)
        {
            high_y = high_y - 2;
        }
        if (high_x == in->image.width)
        {
            high_x = high_x - 2;
        }

        uint8x16_t vPrv[5], vCur[5], vNxt[5], vG[8];

        for (y = low_y; y < high_y; y += in->addr->step_y)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + (y - 2) * in->addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;
            for (vx_uint8 idx = 0; idx < 5; idx++)
            {
                vPrv[idx] = vdupq_n_u8(0);
                vCur[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y);
                vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + 16 * in->addr->stride_x);
            }
            for (x = 0; x < high_x; x += 16)
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
                    vNxt[idx] = vld1q_u8(ptr_src + idx * in->addr->stride_y + (x + 32) * in->addr->stride_x);
                }
            }
         }
     }
}

void LBP_image_tiling_fast(void * VX_RESTRICT parameters[], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_enum *format = (vx_enum *)parameters[1];
    vx_int8 *size = (vx_int8 *)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];

    switch (*format)
    {
    case VX_LBP:
        vxLBPStandard_tiling_fast(in, *size, out);
        break;
    case VX_MLBP:
        vxLBPModified_tiling_fast(in, out);
        break;
    case VX_ULBP:
        vxLBPUniform_tiling_fast(in, *size, out);
        break;
    }
}

vx_uint8 vx_lbp_s(vx_int16 x)
{
    if (x >= 0)
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
    for (vx_int8 p = 1; p < 8; p++)
    {
        u1 = vx_lbp_s(g[p] - gc);
        u2 = vx_lbp_s(g[p - 1] - gc);
        abs2 += abs(u1 - u2);
    }

    return abs1 + abs2;
}

#define LBPSTANDARD_3x3(low_y, high_y, low_x, high_x)                                           \
    for (y = low_y; y < high_y; y += in->addr->step_y)                                          \
    {                                                                                           \
        for (x = low_x; x < high_x; x += in->addr->step_x)                                      \
        {                                                                                       \
            g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y - 1, in->addr);   \
            g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y - 1, in->addr);       \
            g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y - 1, in->addr);   \
            g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y, in->addr);       \
            g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y + 1, in->addr);   \
            g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y + 1, in->addr);       \
            g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y + 1, in->addr);   \
            g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y, in->addr);       \
            gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);             \
                                                                                                \
            sum = 0;                                                                            \
            for (vx_int8 p = 0; p < 8; p++)                                                     \
            {                                                                                   \
                sum += vx_lbp_s(g[p] - gc) * (1 << p);                                          \
            }                                                                                   \
                                                                                                \
            vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);         \
            *dst_ptr = sum;                                                                     \
        }                                                                                       \
    }                                                                                           


#define LBPSTANDARD_5x5(low_y, high_y, low_x, high_x)                                           \
    for (y = low_y; y < high_y; y += in->addr->step_y)                                          \
    {                                                                                           \
        for (x = low_x; x < high_x; x += in->addr->step_x)                                      \
        {                                                                                       \
            g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y - 1, in->addr);   \
            g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y - 2, in->addr);       \
            g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y - 1, in->addr);   \
            g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 2, y, in->addr);       \
            g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y + 1, in->addr);   \
            g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y + 2, in->addr);       \
            g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y + 1, in->addr);   \
            g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 2, y, in->addr);       \
            gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);             \
                                                                                                \
            sum = 0;                                                                            \
            for (vx_int8 p = 0; p < 8; p++)                                                     \
            {                                                                                   \
                sum += vx_lbp_s(g[p] - gc) * (1 << p);                                          \
            }                                                                                   \
                                                                                                \
            vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);         \
            *dst_ptr = sum;                                                                     \
        }                                                                                       \
    }                                                                                           

static void vxLBPStandard_tiling_flexible(vx_tile_ex_t *in, vx_int8 ksize, vx_tile_ex_t *out)
{
    vx_uint32 x = 0, y = 0;

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_uint8 gc, g[8], sum;

    if (low_y == 0 && low_x == 0)
    {
        if (ksize == 3)
            LBPSTANDARD_3x3(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
        else if (ksize == 5)
            LBPSTANDARD_5x5(low_y + 2, high_y - 2, low_x + 2, high_x - 2)
    }
    else
    {
        if (ksize == 3)
        {
            LBPSTANDARD_3x3(1, low_y, low_x, high_x - 1)
            LBPSTANDARD_3x3(low_y, high_y, 1, high_x - 1)
        }
        else if (ksize == 5)
        {
            LBPSTANDARD_5x5(2, low_y, low_x, high_x - 2)
            LBPSTANDARD_5x5(low_y, high_y, 2, high_x - 2)
        }
    }
}

#define LBPMODIFIED(low_y, high_y, low_x, high_x)                                               \
    for (y = low_y; y < high_y; y += in->addr->step_y)                                          \
    {                                                                                           \
        for (x = low_x; x < high_x; x += in->addr->step_x)                                      \
        {                                                                                       \
            g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 2, y - 2, in->addr);   \
            g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y - 2, in->addr);       \
            g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 2, y - 2, in->addr);   \
            g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 2, y, in->addr);       \
            g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 2, y + 2, in->addr);   \
            g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y + 2, in->addr);       \
            g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 2, y + 2, in->addr);   \
            g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 2, y, in->addr);       \
                                                                                                \
            avg = (g[0] + g[1] + g[2] + g[3] + g[4] + g[5] + g[6] + g[7] + 1) / 8;              \
                                                                                                \
            sum = 0;                                                                            \
            for (vx_int8 p = 0; p < 8; p++)                                                     \
            {                                                                                   \
                sum += ((g[p] > avg) * (1 << p));                                               \
            }                                                                                   \
                                                                                                \
            vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);         \
            *dst_ptr = sum;                                                                     \
        }                                                                                       \
    }

void vxLBPModified_tiling_flexible(vx_tile_ex_t *in, vx_tile_ex_t *out)
{
    vx_uint32 x = 0, y = 0;

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_uint8 avg, g[8], sum;

    if (low_y == 0 && low_x == 0)
    {
        LBPMODIFIED(low_y + 2, high_y - 2, low_x + 2, high_x - 2)
    }
    else
    {
        LBPMODIFIED(2, low_y, low_x, high_x - 2)
        LBPMODIFIED(low_y, high_y, 2, high_x - 2)
    }
}

#define LBPUNIFORM_3x3(low_y, high_y, low_x, high_x)                                            \
    for (y = low_y; y < high_y; y += in->addr->step_y)                                          \
    {                                                                                           \
        for (x = low_x; x < high_x; x += in->addr->step_x)                                      \
        {                                                                                       \
            g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y - 1, in->addr);   \
            g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y - 1, in->addr);       \
            g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y - 1, in->addr);   \
            g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y, in->addr);       \
            g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y + 1, in->addr);   \
            g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y + 1, in->addr);       \
            g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y + 1, in->addr);   \
            g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y, in->addr);       \
            gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);             \
                                                                                                \
            sum = 0;                                                                            \
            if (vx_lbp_u(g, gc) <= 2)                                                           \
            {                                                                                   \
                for (vx_uint8 p = 0; p < 8; p++)                                                \
                {                                                                               \
                    sum += vx_lbp_s(g[p] - gc)*(1 << p);                                        \
                }                                                                               \
            }                                                                                   \
            else                                                                                \
            {                                                                                   \
                sum = 9;                                                                        \
            }                                                                                   \
                                                                                                \
            vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);         \
            *dst_ptr = sum;                                                                     \
        }                                                                                       \
    }


#define LBPUNIFORM_5x5(low_y, high_y, low_x, high_x)                                            \
    for (y = low_y; y < high_y; y += in->addr->step_y)                                          \
    {                                                                                           \
        for (x = low_x; x < high_x; x += in->addr->step_x)                                      \
        {                                                                                       \
            g[0] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y - 1, in->addr);   \
            g[1] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y - 2, in->addr);       \
            g[2] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y - 1, in->addr);   \
            g[3] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 2, y, in->addr);       \
            g[4] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x + 1, y + 1, in->addr);   \
            g[5] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y + 2, in->addr);       \
            g[6] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 1, y + 1, in->addr);   \
            g[7] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x - 2, y, in->addr);       \
            gc = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);             \
                                                                                                \
            sum = 0;                                                                            \
            if (vx_lbp_u(g, gc) <= 2)                                                           \
            {                                                                                   \
                for (vx_uint8 p = 0; p < 8; p++)                                                \
                {                                                                               \
                    sum += vx_lbp_s(g[p] - gc)*(1 << p);                                        \
                }                                                                               \
            }                                                                                   \
            else                                                                                \
            {                                                                                   \
                sum = 9;                                                                        \
            }                                                                                   \
                                                                                                \
            vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);         \
            *dst_ptr = sum;                                                                     \
        }                                                                                       \
    }


void vxLBPUniform_tiling_flexible(vx_tile_ex_t *in, vx_int8 ksize, vx_tile_ex_t *out)
{
    vx_uint32 x = 0, y = 0;

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_uint8 gc, g[8], sum;

    if (low_y == 0 && low_x == 0)
    {
        if (ksize == 3)
            LBPUNIFORM_3x3(low_y + 1, high_y - 1, low_x + 1, high_x - 1)
        else if (ksize == 5)
            LBPUNIFORM_5x5(low_y + 2, high_y - 2, low_x + 2, high_x - 2)
    }
    else
    {
        if (ksize == 3)
        {
            LBPUNIFORM_3x3(1, low_y, low_x, high_x - 1)
            LBPUNIFORM_3x3(low_y, high_y, 1, high_x - 1)
        }
        else if (ksize == 5)
        {
            LBPUNIFORM_5x5(2, low_y, low_x, high_x - 2)
            LBPUNIFORM_5x5(low_y, high_y, 2, high_x - 2)
        }
    }
}


void LBP_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_enum *format = (vx_enum *)parameters[1];
    vx_int8 *size = (vx_int8 *)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];

    switch (*format)
    {
    case VX_LBP:
        vxLBPStandard_tiling_flexible(in, *size, out);
        break;
    case VX_MLBP:
        vxLBPModified_tiling_flexible(in, out);
        break;
    case VX_ULBP:
        vxLBPUniform_tiling_flexible(in, *size, out);
        break;
    }
}
