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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "venum.h"

#define VX_SCALE_PYRAMID_DOUBLE (2.0f)

static vx_status copyImage2Mem(vx_image input, vx_int16 *dst)
{
    vx_status status = VX_SUCCESS; // assume success until an error occurs.
    vx_uint32 p = 0;
    vx_uint32 y = 0, x = 0;
    vx_size planes = 0;

    void *src;
    vx_imagepatch_addressing_t src_addr;
    vx_rectangle_t src_rect;
    vx_map_id map_id1;
    vx_df_image src_format = 0;
    vx_uint32 wWidth;

    status |= vxQueryImage(input, VX_IMAGE_PLANES, &planes, sizeof(planes));
    vxQueryImage(input, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxGetValidRegionImage(input, &src_rect);
    for (p = 0; p < planes && status == VX_SUCCESS; p++)
    {
        status = VX_SUCCESS;
        src = NULL;

        status |= vxMapImagePatch(input, &src_rect, p, &map_id1, &src_addr, &src, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

        wWidth = (src_addr.dim_x >> 3) << 3;
        for (y = 0; y < src_addr.dim_y && status == VX_SUCCESS; y += src_addr.step_y)
        {
            vx_int16 *ptr_dst = dst + y * src_addr.dim_x;
            vx_uint8 *ptr_src = (vx_uint8 *)src + y * src_addr.stride_y;
            for (x = 0; x < wWidth; x += 8)
            {
                if (src_format == VX_DF_IMAGE_U8)
                {
                    uint8x8_t vSrc = vld1_u8(ptr_src + x * src_addr.stride_x);
                    uint16x8_t vSrcu16 = vmovl_u8(vSrc);
                    int16x8_t vSrcs16 = vcombine_s16(vreinterpret_s16_u16(vget_low_u16(vSrcu16)),
                        vreinterpret_s16_u16(vget_high_u16(vSrcu16)));
                    vst1q_s16(ptr_dst + x, vSrcs16);
                }
                else if (src_format == VX_DF_IMAGE_S16)
                {
                    int16x8_t vSrc = vld1q_s16((vx_int16 *)(ptr_src + x * src_addr.stride_x));
                    vst1q_s16(ptr_dst + x, vSrc);
                }
            }
            for (x = wWidth; x < src_addr.dim_x; x += src_addr.step_x)
            {
                void* srcp = vxFormatImagePatchAddress2d(src, x, y, &src_addr);
                vx_int32 out0 = src_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)srcp : *(vx_int16 *)srcp;
                ptr_dst[x] = out0;
            }
        }

        if (status == VX_SUCCESS)
        {
            status |= vxUnmapImagePatch(input, map_id1);
        }
    }

    return status;
}

static vx_status copyMem2Image(vx_int16 *src, vx_uint32 src_width, vx_image output)
{
    vx_status status = VX_SUCCESS; // assume success until an error occurs.
    vx_uint32 y = 0, x = 0;

    void *dst_base = NULL;
    vx_imagepatch_addressing_t dst_addr;
    vx_rectangle_t dst_rect;
    vx_df_image dst_format = 0;
    int16x8_t vMaxu8 = vdupq_n_s16(UINT8_MAX);
    int16x8_t vMinu8 = vdupq_n_s16(0);
    vx_uint32 w8;

    vxQueryImage(output, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    status = vxGetValidRegionImage(output, &dst_rect);
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(output, &dst_rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    w8 = (dst_addr.dim_x >> 3) << 3;
    for (y = 0; y < dst_addr.dim_y && status == VX_SUCCESS; y += dst_addr.step_y)
    {
        vx_int16 *ptr_src = src + y * src_width;
        vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
        for (x = 0; x < w8; x += 8)
        {
            int16x8_t vSrc = vld1q_s16(ptr_src + x);
            if (dst_format == VX_DF_IMAGE_S16)
            {
                vst1q_s16((vx_int16 *)(ptr_dst + x * dst_addr.stride_x), vSrc);
            }
            else if (dst_format == VX_DF_IMAGE_U8)
            {
                vSrc = vminq_s16(vSrc, vMaxu8);
                vSrc = vmaxq_s16(vSrc, vMinu8);
                vst1_u8(ptr_dst + x * dst_addr.stride_x, vqmovun_s16(vSrc));
            }
        }
        for (x = w8; x < dst_addr.dim_x; x += dst_addr.step_x)
        {
            void* dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            vx_int16 out0 = ptr_src[x];
            if (dst_format == VX_DF_IMAGE_S16)
                *(vx_int16 *)dstp = out0;
            else if (dst_format == VX_DF_IMAGE_U8)
                *(vx_uint8 *)dstp = out0 < 0 ? 0 : (out0 > UINT8_MAX ? UINT8_MAX : (vx_uint8)out0);
        }
    }
    if (status == VX_SUCCESS)
    {
        status |= vxUnmapImagePatch(output, map_id_dst);
    }
    return status;
}

static void calcgauss5x5(uint8x16_t *vPrv, uint8x16_t *vCur, uint8x16_t *vNxt,
                         uint16x8_t *vSumu16_low, uint16x8_t *vSumu16_high)
{
    *vSumu16_low = vaddq_u16(vmovl_u8(vget_low_u8(vCur[0])), vmovl_u8(vget_low_u8(vCur[4])));
    *vSumu16_high = vaddq_u16(vmovl_u8(vget_high_u8(vCur[0])), vmovl_u8(vget_high_u8(vCur[4])));
    uint8x16_t vFront = vextq_u8(vPrv[2], vCur[2], 14);
    uint8x16_t vBack = vextq_u8(vCur[2], vNxt[2], 2);
    *vSumu16_low = vaddq_u16(*vSumu16_low, vmovl_u8(vget_low_u8(vFront)));
    *vSumu16_low = vaddq_u16(*vSumu16_low, vmovl_u8(vget_low_u8(vBack)));
    *vSumu16_high = vaddq_u16(*vSumu16_high, vmovl_u8(vget_high_u8(vFront)));
    *vSumu16_high = vaddq_u16(*vSumu16_high, vmovl_u8(vget_high_u8(vBack)));
    *vSumu16_low = vmulq_n_u16(*vSumu16_low, 6);
    *vSumu16_high = vmulq_n_u16(*vSumu16_high, 6);

    vFront = vextq_u8(vPrv[0], vCur[0], 15);
    vBack = vextq_u8(vCur[0], vNxt[0], 1);
    uint16x8_t vTmpSumu16_low = vaddq_u16(vmovl_u8(vget_low_u8(vFront)), vmovl_u8(vget_low_u8(vBack)));
    uint16x8_t vTmpSumu16_high = vaddq_u16(vmovl_u8(vget_high_u8(vFront)), vmovl_u8(vget_high_u8(vBack)));
    vFront = vextq_u8(vPrv[4], vCur[4], 15);
    vBack = vextq_u8(vCur[4], vNxt[4], 1);
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vFront)));
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vFront)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vBack)));
    vFront = vextq_u8(vPrv[1], vCur[1], 14);
    vBack = vextq_u8(vCur[1], vNxt[1], 2);
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vFront)));
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vFront)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vBack)));
    vFront = vextq_u8(vPrv[3], vCur[3], 14);
    vBack = vextq_u8(vCur[3], vNxt[3], 2);
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vFront)));
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vFront)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vBack)));
    vTmpSumu16_low = vshlq_n_u16(vTmpSumu16_low, 2);
    vTmpSumu16_high = vshlq_n_u16(vTmpSumu16_high, 2);
    *vSumu16_low = vaddq_u16(*vSumu16_low, vTmpSumu16_low);
    *vSumu16_high = vaddq_u16(*vSumu16_high, vTmpSumu16_high);

    vFront = vextq_u8(vPrv[1], vCur[1], 15);
    vBack = vextq_u8(vCur[1], vNxt[1], 1);
    vTmpSumu16_low = vaddq_u16(vmovl_u8(vget_low_u8(vFront)), vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vmovl_u8(vget_high_u8(vFront)), vmovl_u8(vget_high_u8(vBack)));
    vFront = vextq_u8(vPrv[3], vCur[3], 15);
    vBack = vextq_u8(vCur[3], vNxt[3], 1);
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vFront)));
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vFront)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vBack)));
    vTmpSumu16_low = vshlq_n_u16(vTmpSumu16_low, 4);
    vTmpSumu16_high = vshlq_n_u16(vTmpSumu16_high, 4);
    *vSumu16_low = vaddq_u16(*vSumu16_low, vTmpSumu16_low);
    *vSumu16_high = vaddq_u16(*vSumu16_high, vTmpSumu16_high);

    vFront = vextq_u8(vPrv[2], vCur[2], 15);
    vBack = vextq_u8(vCur[2], vNxt[2], 1);
    vTmpSumu16_low = vaddq_u16(vmovl_u8(vget_low_u8(vFront)), vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vmovl_u8(vget_high_u8(vFront)), vmovl_u8(vget_high_u8(vBack)));
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vCur[1])));
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vCur[3])));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vCur[1])));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vCur[3])));
    vTmpSumu16_low = vmulq_n_u16(vTmpSumu16_low, 24);
    vTmpSumu16_high = vmulq_n_u16(vTmpSumu16_high, 24);
    *vSumu16_low = vaddq_u16(*vSumu16_low, vTmpSumu16_low);
    *vSumu16_high = vaddq_u16(*vSumu16_high, vTmpSumu16_high);

    *vSumu16_low = vmlaq_n_u16(*vSumu16_low, vmovl_u8(vget_low_u8(vCur[2])), 36);
    *vSumu16_high = vmlaq_n_u16(*vSumu16_high, vmovl_u8(vget_high_u8(vCur[2])), 36);

    vFront = vextq_u8(vPrv[0], vCur[0], 14);
    vBack = vextq_u8(vCur[0], vNxt[0], 2);
    vTmpSumu16_low = vaddq_u16(vmovl_u8(vget_low_u8(vFront)), vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vmovl_u8(vget_high_u8(vFront)), vmovl_u8(vget_high_u8(vBack)));
    vFront = vextq_u8(vPrv[4], vCur[4], 14);
    vBack = vextq_u8(vCur[4], vNxt[4], 2);
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vFront)));
    vTmpSumu16_low = vaddq_u16(vTmpSumu16_low, vmovl_u8(vget_low_u8(vBack)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vFront)));
    vTmpSumu16_high = vaddq_u16(vTmpSumu16_high, vmovl_u8(vget_high_u8(vBack)));
    *vSumu16_low = vaddq_u16(*vSumu16_low, vTmpSumu16_low);
    *vSumu16_high = vaddq_u16(*vSumu16_high, vTmpSumu16_high);

    *vSumu16_low = vshrq_n_u16(*vSumu16_low, 8);
    *vSumu16_high = vshrq_n_u16(*vSumu16_high, 8);

    //sum x 4
    *vSumu16_low = vshlq_n_u16(*vSumu16_low, 2);
    *vSumu16_high = vshlq_n_u16(*vSumu16_high, 2);
}

static void reconguass5x5(vx_uint8 *src, vx_uint32 width, vx_uint32 height, vx_int16 *pyramid,
                          vx_int16 *dst)
{
    vx_int32 y, x;
    vx_uint32 w16 = (width >> 4) << 4;
    int32x4_t vMaxs16 = vdupq_n_s32(INT16_MAX);
    int32x4_t vMins16 = vdupq_n_s32(INT16_MIN);
    uint8x16_t vPrv[5], vCur[5], vNxt[5];
    vx_uint8 szTmpData[16];

    for (x = 0; x < w16; x += 16)
    {
        vx_int16 *ptr_dst = dst + x;
        vx_int16 *ptr_pyr = pyramid + x;
        for (vx_int32 radius = -2, idx = 0; radius <= 2; (radius++, idx++))
        {
            vx_int32 tmpY = radius;
            if (radius < 0)
            {
                tmpY = 0;
            }
            if (0 == x)
            {
                vPrv[idx] = vdupq_n_u8(*(src + tmpY * width));
            }
            else
            {
                vPrv[idx] = vld1q_u8(src + tmpY * width + (x - 16));
            }
            vCur[idx] = vld1q_u8(src + (tmpY * width) + x);
            if ((x + 16) >= w16)
            {
                memcpy(szTmpData, src + tmpY * width + (x + 16), width - (x + 16));
                memset(&szTmpData[width - (x + 16)],
                       *(src + tmpY * width + width - 1),
                       16 - (width - (x + 16)));
                vNxt[idx] = vld1q_u8(szTmpData);
            }
            else
            {
                vNxt[idx] = vld1q_u8(src + tmpY * width + (x + 16));
            }
        }

        for (y = 0; y < height; y++)
        {
            uint16x8_t vSumu16_low = vdupq_n_u16(0);
            uint16x8_t vSumu16_high= vdupq_n_u16(0);
            calcgauss5x5(vPrv, vCur, vNxt, &vSumu16_low, &vSumu16_high);
            int16x8_t vSums16 = vcombine_s16(vreinterpret_s16_u16(vget_low_u16(vSumu16_low)),
                vreinterpret_s16_u16(vget_high_u16(vSumu16_low)));
            int16x8_t vPyr = vld1q_s16(ptr_pyr + y * width);
            int32x4_t vSums32_low = vaddq_s32(vmovl_s16(vget_low_s16(vSums16)), vmovl_s16(vget_low_s16(vPyr)));
            int32x4_t vSums32_high = vaddq_s32(vmovl_s16(vget_high_s16(vSums16)), vmovl_s16(vget_high_s16(vPyr)));
            vSums32_low = vminq_s32(vSums32_low, vMaxs16);
            vSums32_low = vmaxq_s32(vSums32_low, vMins16);
            vSums32_high = vminq_s32(vSums32_high, vMaxs16);
            vSums32_high = vmaxq_s32(vSums32_high, vMins16);
            vSums16 = vcombine_s16(vmovn_s32(vSums32_low), vmovn_s32(vSums32_high));
            vst1q_s16(ptr_dst + y * width, vSums16);

            vSums16 = vcombine_s16(vreinterpret_s16_u16(vget_low_u16(vSumu16_high)),
                vreinterpret_s16_u16(vget_high_u16(vSumu16_high)));
            vPyr = vld1q_s16(ptr_pyr + y * width + 8);
            vSums32_low = vaddq_s32(vmovl_s16(vget_low_s16(vSums16)), vmovl_s16(vget_low_s16(vPyr)));
            vSums32_high = vaddq_s32(vmovl_s16(vget_high_s16(vSums16)), vmovl_s16(vget_high_s16(vPyr)));
            vSums32_low = vminq_s32(vSums32_low, vMaxs16);
            vSums32_low = vmaxq_s32(vSums32_low, vMins16);
            vSums32_high = vminq_s32(vSums32_high, vMaxs16);
            vSums32_high = vmaxq_s32(vSums32_high, vMins16);
            vSums16 = vcombine_s16(vmovn_s32(vSums32_low), vmovn_s32(vSums32_high));
            vst1q_s16(ptr_dst + y * width + 8, vSums16);

            for (vx_int32 idx = 0; idx < 4; idx++)
            {
                vPrv[idx] = vPrv[idx + 1];
                vCur[idx] = vCur[idx + 1];
                vNxt[idx] = vNxt[idx + 1];
            }
            if ((y + 2 + 1) < (height + 2))
            {
                vx_int32 nxtY = y + 2 + 1;
                if ((y + 2 + 1) >= height)
                {
                    nxtY = height - 1;
                }

                if (0 == x)
                {
                    vPrv[4] = vdupq_n_u8(*(src + nxtY * width));
                }
                else
                {
                    vPrv[4] = vld1q_u8(src + nxtY * width + x - 16);
                }
                vCur[4] = vld1q_u8(src + nxtY * width + x);
                if ((x + 16) >= w16)
                {
                    memcpy(szTmpData, src + nxtY * width + (x + 16), width - (x + 16));
                    memset(&szTmpData[width - (x + 16)],
                           *(src + nxtY * width + width - 1),
                           16 - (width - (x + 16)));
                    vNxt[4] = vld1q_u8(szTmpData);
                }
                else
                {
                    vNxt[4] = vld1q_u8(src + nxtY * width + (x + 16));
                }
            }
        }
    }
    if (w16 < width)
    {
        vx_int16 *ptr_dst = dst + w16;
        vx_int16 *ptr_pyr = pyramid + w16;
        vx_int16 szDstData[8];
        for (vx_int32 radius = -2, idx = 0; radius <= 2; (radius++, idx++))
        {
            vx_int32 tmpY = radius;
            if (radius < 0)
            {
                tmpY = 0;
            }

            vPrv[idx] = vld1q_u8(src + tmpY * width + (x - 16));
            memcpy(szTmpData, src + tmpY * width + w16, width - w16);
            memset(&szTmpData[width - w16],
                   *(src + tmpY * width + width - 1),
                   16 - (width - w16));
            vCur[idx] = vld1q_u8(szTmpData);
            vNxt[idx] = vdupq_n_u8(szTmpData[15]);
        }

        for (y = 0; y < height; y++)
        {
            uint16x8_t vSumu16_low = vdupq_n_u16(0);
            uint16x8_t vSumu16_high= vdupq_n_u16(0);
            calcgauss5x5(vPrv, vCur, vNxt, &vSumu16_low, &vSumu16_high);
            int16x8_t vSums16 = vcombine_s16(vreinterpret_s16_u16(vget_low_u16(vSumu16_low)),
                vreinterpret_s16_u16(vget_high_u16(vSumu16_low)));
            if ((width - w16) > 8)
            {
                int16x8_t vPyr = vld1q_s16(ptr_pyr + y * width);
                int32x4_t vSums32_low = vaddq_s32(vmovl_s16(vget_low_s16(vSums16)), vmovl_s16(vget_low_s16(vPyr)));
                int32x4_t vSums32_high = vaddq_s32(vmovl_s16(vget_high_s16(vSums16)), vmovl_s16(vget_high_s16(vPyr)));
                vSums32_low = vminq_s32(vSums32_low, vMaxs16);
                vSums32_low = vmaxq_s32(vSums32_low, vMins16);
                vSums32_high = vminq_s32(vSums32_high, vMaxs16);
                vSums32_high = vmaxq_s32(vSums32_high, vMins16);
                vSums16 = vcombine_s16(vmovn_s32(vSums32_low), vmovn_s32(vSums32_high));
                vst1q_s16(ptr_dst + y * width, vSums16);

                vSums16 = vcombine_s16(vreinterpret_s16_u16(vget_low_u16(vSumu16_high)),
                    vreinterpret_s16_u16(vget_high_u16(vSumu16_high)));
                vst1q_s16(szDstData, vSums16);
                for (vx_int32 idx = 0; idx < (width - w16 - 8); idx++)
                {
                    vx_int32 tmpOut = ptr_pyr[y * width + 8 + idx] + szDstData[idx];
                    tmpOut = tmpOut < INT16_MIN ? INT16_MIN : (tmpOut > INT16_MAX ? INT16_MAX : tmpOut);
                    ptr_dst[y * width + 8 + idx] = (vx_int16)tmpOut;
                }

            }
            else
            {
                vst1q_s16(szDstData, vSums16);
                for (vx_int32 idx = 0; idx < (width - w16); idx++)
                {
                    vx_int32 tmpOut = ptr_pyr[y * width + idx] + szDstData[idx];
                    tmpOut = tmpOut < INT16_MIN ? INT16_MIN : (tmpOut > INT16_MAX ? INT16_MAX : tmpOut);
                    ptr_dst[y * width + idx] = (vx_int16)tmpOut;
                }
            }

            for (vx_int32 idx = 0; idx < 4; idx++)
            {
                vPrv[idx] = vPrv[idx + 1];
                vCur[idx] = vCur[idx + 1];
                vNxt[idx] = vNxt[idx + 1];
            }
            if ((y + 2 + 1) < (height + 2))
            {
                vx_int32 nxtY = y + 2 + 1;
                if ((y + 2 + 1) >= height)
                {
                    nxtY = height - 1;
                }


                vPrv[4] = vld1q_u8(src + nxtY * width + x - 16);
                memcpy(szTmpData, src + nxtY * width + w16, width - w16);
                memset(&szTmpData[width - w16],
                       *(src + nxtY * width + width - 1),
                       16 - (width - w16));
                vCur[4] = vld1q_u8(szTmpData);
                vNxt[4] = vdupq_n_u8(szTmpData[15]);
            }
        }
    }
}

static void reconstructImage(vx_int16 *src, vx_uint32 width, vx_uint32 height, vx_int16 *pyramid,
                             vx_uint8 *mid, vx_int16 *dst)
{
    vx_int32 iy, ix;
    vx_int32 sum, out;
    vx_uint32 w16 = (width >> 4) << 4;
    int16x8_t vMaxu8 = vdupq_n_s16(UINT8_MAX);
    int16x8_t vZero = vdupq_n_s16(0);

    for (iy = 0; iy < height; iy++)
    {
        vx_int16 *ptr_src = src + (iy >> 1) * (width >> 1);
        vx_uint8 *ptr_mid = mid + iy * width;
        for (ix = 0; ix < w16; ix+=16)
        {
            if (iy % 2 != 0)
            {
                vst1q_u8(ptr_mid + ix, vdupq_n_u8(0));
            }
            else
            {
                int16x8_t vSrc = vld1q_s16((vx_int16 *)((vx_uint8 *)ptr_src + ix));
                vSrc = vminq_s16(vSrc, vMaxu8);
                vSrc = vmaxq_s16(vSrc, vZero);
                uint8x8_t vSrcu8 = vreinterpret_u8_s8(vmovn_s16(vSrc));
                uint8x8_t vZerou8 = vdup_n_u8(0);
                uint8x8x2_t pixels = {{vSrcu8, vZerou8}};
                vst2_u8(ptr_mid + ix, pixels);
            }
        }
        for (ix = w16; ix < width; ix++)
        {
            if (iy % 2 != 0 || ix % 2 != 0)
            {
                ptr_mid[ix] = 0;
            }
            else
            {
                vx_int32 filling_data = src[(iy/2) * (width/2) + (ix / 2)];
                if (filling_data > UINT8_MAX)
                    filling_data = UINT8_MAX;
                else if (filling_data < 0)
                    filling_data = 0;
                ptr_mid[ix] = (vx_uint8)filling_data;
            }
        }
    }

    //calculate convolve, gaussian 5x5 filter
    reconguass5x5(mid, width, height, pyramid, dst);

    return;
}

vx_status vxLaplacianReconstruct(vx_pyramid pyramid, vx_image src, vx_image dst)
{
    vx_status status = VX_SUCCESS;
    vx_size lev;
    vx_size levels = 1;
    vx_uint32 width = 0;
    vx_uint32 height = 0;
    vx_uint32 level_width = 0;
    vx_uint32 level_height = 0;
    vx_int16 *filling, *out;
    vx_uint8 *mid;
    vx_image pyr_level = 0;
    vx_uint32 len;
    vx_uint32 reconstructor;

    status = vxQueryImage(src, VX_IMAGE_WIDTH, &width, sizeof(width));
    status |= vxQueryImage(src, VX_IMAGE_HEIGHT, &height, sizeof(height));
    status |= vxQueryPyramid(pyramid, VX_PYRAMID_LEVELS, &levels, sizeof(levels));
    level_width = (vx_uint32)ceilf(width  * VX_SCALE_PYRAMID_DOUBLE);
    level_height = (vx_uint32)ceilf(height * VX_SCALE_PYRAMID_DOUBLE);
    reconstructor = (vx_uint32)pow(VX_SCALE_PYRAMID_DOUBLE, levels);
    len = (width * reconstructor) * (height * reconstructor) * sizeof(vx_int16);

    filling = (vx_int16 *)malloc(len);
    out = (vx_int16 *)malloc(len);
    mid = (vx_uint8 *)malloc(len >> 1);
    if (NULL == filling || NULL == out || NULL == mid)
    {
        if (NULL != filling)
        {
            free(filling);
        }
        if (NULL != out)
        {
            free(out);
        }
        if (NULL != mid)
        {
            free(mid);
        }
        return VX_FAILURE;
    }

    copyImage2Mem(src, filling);
    for (lev = 0; lev < levels; lev++)
    {
        vx_rectangle_t rect;
        vx_imagepatch_addressing_t pyr_addr;
        void *pyr_base = NULL;
        pyr_level = vxGetPyramidLevel(pyramid, (vx_uint32)((levels - 1) - lev));
        status = vxGetValidRegionImage(pyr_level, &rect);
        vx_map_id map_id_pyr = 0;
        status |= vxMapImagePatch(pyr_level, &rect, 0, &map_id_pyr, &pyr_addr, (void **)&pyr_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        reconstructImage(filling, level_width, level_height, pyr_base, mid, out);
        status |= vxUnmapImagePatch(pyr_level, map_id_pyr);
        status |= vxReleaseImage(&pyr_level);

        if ((levels - 1) - lev == 0)
        {
            copyMem2Image(out, level_width, dst);
        }
        else
        {
            vx_int16 *ptr_tmp;
            ptr_tmp = filling;
            filling = out;
            out = ptr_tmp;

            level_width = (vx_uint32)ceilf(level_width  * VX_SCALE_PYRAMID_DOUBLE);
            level_height = (vx_uint32)ceilf(level_height * VX_SCALE_PYRAMID_DOUBLE);
        }
    }
    free(filling);
    free(out);
    free(mid);
    return status;
}

