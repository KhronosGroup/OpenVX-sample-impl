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

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// nodeless version of the MeanStdDev kernel
vx_status vxMeanStdDev_U8(vx_image input, vx_scalar mean, vx_scalar stddev)
{
    vx_float32 fmean = 0.0f; vx_float64 sum = 0.0, sum_diff_sqrs = 0.0; vx_float32 fstddev = 0.0f;
    vx_df_image format = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t addrs = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    void *base_ptr = NULL;
    vx_uint32 x, y;
    vx_status status  = VX_SUCCESS;

    vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
    status = vxGetValidRegionImage(input, &rect);
    //VX_PRINT(VX_ZONE_INFO, "Rectangle = {%u,%u x %u,%u}\n",rect.start_x, rect.start_y, rect.end_x, rect.end_y);
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &addrs, &base_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

    if (format == VX_DF_IMAGE_U8)
    {
        if (addrs.stride_y == (ptrdiff_t)(addrs.dim_x))
        {
            addrs.dim_x *= addrs.dim_y;
            addrs.dim_y = 1;
        }
        const size_t width = addrs.dim_x;
        size_t blockSize0 = 1 << 23;
        size_t roiw8 = width & ~7;

        uint32x4_t v_zero = vdupq_n_u32(0u);

        for (size_t i = 0; i < addrs.dim_y; ++i)
        {
            const vx_uint8 * src = (vx_uint8 *)base_ptr + i * addrs.stride_y;
            size_t j = 0u;

            while (j < roiw8)
            {
                size_t blockSize = MIN(roiw8 - j, blockSize0) + j;
                uint32x4_t v_sum = v_zero;
                uint32x4_t v_sqsum = v_zero;

                for ( ; j < blockSize ; j += 8, src += 8)
                {
                    uint8x8_t v_src0 = vld1_u8(src);

                    uint16x8_t v_src = vmovl_u8(v_src0);
                    uint16x4_t v_srclo = vget_low_u16(v_src), v_srchi = vget_high_u16(v_src);
                    v_sum = vaddq_u32(v_sum, vaddl_u16(v_srclo, v_srchi));
                    v_sqsum = vmlal_u16(v_sqsum, v_srclo, v_srclo);
                    v_sqsum = vmlal_u16(v_sqsum, v_srchi, v_srchi);
                }

                vx_uint32 arsum[8];
                vst1q_u32(arsum, v_sum);
                vst1q_u32(arsum + 4, v_sqsum);

                sum += (vx_float64)arsum[0];
                sum += (vx_float64)arsum[1];
                sum += (vx_float64)arsum[2];
                sum += (vx_float64)arsum[3];
                sum_diff_sqrs += (vx_float64)arsum[4];
                sum_diff_sqrs += (vx_float64)arsum[5];
                sum_diff_sqrs += (vx_float64)arsum[6];
                sum_diff_sqrs += (vx_float64)arsum[7];
            }

            for ( ; j < width; j+=1, src+=1)
            {
                vx_uint32 srcval = src[0];
                sum += srcval;
                sum_diff_sqrs += srcval * srcval;
            }
        }
    }
    else if (format == VX_DF_IMAGE_U16)
    {
        uint32x4_t v_zero = vdupq_n_u32(0u), v_sum;
        float32x4_t v_zero_f = vdupq_n_f32(0.0f), v_sqsum;
        size_t blockSize0 = 1 << 10, roiw4 = addrs.dim_x & ~3;

        vx_float32 arsum[8];

        for (size_t i = 0; i < addrs.dim_y; ++i)
        {
            const vx_uint16 * src = (vx_uint16 *)base_ptr + i * addrs.stride_y / 2;
            size_t j = 0u;

            while (j < roiw4)
            {
                size_t blockSize = MIN(roiw4 - j, blockSize0) + j;
                v_sum = v_zero;
                v_sqsum = v_zero_f;

                for ( ; j + 16 < blockSize ; j += 16)
                {
                    uint16x8_t v_src0 = vld1q_u16(src + j), v_src1 = vld1q_u16(src + j + 8);

                    // 0
                    uint32x4_t v_srclo = vmovl_u16(vget_low_u16(v_src0));
                    uint32x4_t v_srchi = vmovl_u16(vget_high_u16(v_src0));
                    v_sum = vaddq_u32(v_sum, vaddq_u32(v_srclo, v_srchi));
                    float32x4_t v_srclo_f = vcvtq_f32_u32(v_srclo);
                    float32x4_t v_srchi_f = vcvtq_f32_u32(v_srchi);
                    v_sqsum = vmlaq_f32(v_sqsum, v_srclo_f, v_srclo_f);
                    v_sqsum = vmlaq_f32(v_sqsum, v_srchi_f, v_srchi_f);

                    // 1
                    v_srclo = vmovl_u16(vget_low_u16(v_src1));
                    v_srchi = vmovl_u16(vget_high_u16(v_src1));
                    v_sum = vaddq_u32(v_sum, vaddq_u32(v_srclo, v_srchi));
                    v_srclo_f = vcvtq_f32_u32(v_srclo);
                    v_srchi_f = vcvtq_f32_u32(v_srchi);
                    v_sqsum = vmlaq_f32(v_sqsum, v_srclo_f, v_srclo_f);
                    v_sqsum = vmlaq_f32(v_sqsum, v_srchi_f, v_srchi_f);
                }

                for ( ; j < blockSize; j += 4)
                {
                    uint32x4_t v_src = vmovl_u16(vld1_u16(src + j));
                    float32x4_t v_src_f = vcvtq_f32_u32(v_src);
                    v_sum = vaddq_u32(v_sum, v_src);
                    v_sqsum = vmlaq_f32(v_sqsum, v_src_f, v_src_f);
                }

                vst1q_f32(arsum, vcvtq_f32_u32(v_sum));
                vst1q_f32(arsum + 4, v_sqsum);

                sum += (vx_float64)arsum[0] + arsum[1] + arsum[2] + arsum[3];
                sum_diff_sqrs += (vx_float64)arsum[4] + arsum[5] + arsum[6] + arsum[7];
            }

            // collect a few last elements in the current row
            for ( ; j < addrs.dim_y; ++j)
            {
                vx_float32 srcval = src[j];
                sum += srcval;
                sum_diff_sqrs += srcval * srcval;
            }
        }
    }

    // calc mean and stddev
    vx_uint32 total_size = addrs.dim_y * addrs.dim_x;
    vx_float32 itotal = 1.0 / total_size;
    fmean = sum * itotal;
    fstddev = sqrt(sum_diff_sqrs * itotal - fmean * fmean);

    status |= vxCopyScalar(mean, &fmean, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(stddev, &fstddev, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    status |= vxUnmapImagePatch(input, map_id);

    return status;
}


vx_status vxMeanStdDev_U1(vx_image input, vx_scalar mean, vx_scalar stddev)
{
    vx_float32 fmean = 0.0f; vx_float64 sum = 0.0, sum_diff_sqrs = 0.0; vx_float32 fstddev = 0.0f;
    vx_df_image format = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t addrs = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id map_id = 0;
    void *base_ptr = NULL;
    vx_uint32 x, y, width, height, shift_x_u1;
    vx_status status  = VX_SUCCESS;

    status  = vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(input, &rect);
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &addrs, &base_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    //VX_PRINT(VX_ZONE_INFO, "Rectangle = {%u,%u x %u,%u}\n",rect.start_x, rect.start_y, rect.end_x, rect.end_y);

    width  = rect.end_x - rect.start_x;
    height = rect.end_y - rect.start_y;
    shift_x_u1 = rect.start_x % 8;    // Bit shift for U1 valid regions

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            vx_uint8 *pixels = vxFormatImagePatchAddress2d(base_ptr, x + shift_x_u1, y, &addrs);
            sum += (*pixels & (1 << (x + shift_x_u1) % 8)) >> (x + shift_x_u1) % 8;
        }
    }
    fmean = (vx_float32)(sum / (width * height));
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            vx_uint8 *pixels = vxFormatImagePatchAddress2d(base_ptr, x + shift_x_u1, y, &addrs);
            vx_uint8  pixel  = (*pixels & (1 << (x + shift_x_u1) % 8)) >> (x + shift_x_u1) % 8;
            sum_diff_sqrs += pow((vx_float64)pixel - fmean, 2);
        }
    }

    fstddev = (vx_float32)sqrt(sum_diff_sqrs / (width * height));

    status |= vxCopyScalar(mean, &fmean, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(stddev, &fstddev, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    status |= vxUnmapImagePatch(input, map_id);

    return status;
}

vx_status vxMeanStdDev(vx_image input, vx_scalar mean, vx_scalar stddev)
{
    vx_df_image format;
    vx_status status = vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1)
        return vxMeanStdDev_U1(input, mean, stddev);
    else
        return vxMeanStdDev_U8(input, mean, stddev);
}

// nodeless version of the MinMaxLoc kernel
static void analyzeMinMaxValue(vx_uint32 x, vx_uint32 y, vx_int64 v,
                               vx_int64 *pMinVal, vx_int64 *pMaxVal,
                               vx_uint32 *pMinCount, vx_uint32 *pMaxCount,
                               vx_array minLoc, vx_array maxLoc)
{
    vx_coordinates2d_t loc;
    vx_size minLocCapacity = 0, maxLocCapacity = 0;

    loc.x = x;
    loc.y = y;

    vxQueryArray(minLoc, VX_ARRAY_CAPACITY, &minLocCapacity, sizeof(minLocCapacity));
    vxQueryArray(maxLoc, VX_ARRAY_CAPACITY, &maxLocCapacity, sizeof(maxLocCapacity));

    if (v > *pMaxVal)
    {
        *pMaxVal = v;
        *pMaxCount = 1;
        if (maxLoc)
        {
            vxTruncateArray(maxLoc, 0);
            vxAddArrayItems(maxLoc, 1, &loc, sizeof(loc));
        }
    }
    else if (v == *pMaxVal)
    {
        if (maxLoc && *pMaxCount < maxLocCapacity)
        {
            vxAddArrayItems(maxLoc, 1, &loc, sizeof(loc));
        }
        ++(*pMaxCount);
    }

    if (v < *pMinVal)
    {
        *pMinVal = v;
        *pMinCount = 1;
        if (minLoc)
        {
            vxTruncateArray(minLoc, 0);
            vxAddArrayItems(minLoc, 1, &loc, sizeof(loc));
        }
    }
    else if (v == *pMinVal)
    {
        if (minLoc && *pMinCount < minLocCapacity)
        {
            vxAddArrayItems(minLoc, 1, &loc, sizeof(loc));
        }
        ++(*pMinCount);
    }
}

static void addLocu8(vx_uint32 y, vx_uint32 x, uint8x16_t *pvPred, vx_array arrLoc)
{
    uint8x16_t vPred = *pvPred;
    vx_uint32 szIdx[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    vx_coordinates2d_t loc;
    loc.y = y;
    if (0 != vgetq_lane_u8(vPred, 0))
    {
        loc.x = x + szIdx[0];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 1))
    {
        loc.x = x + szIdx[1];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 2))
    {
        loc.x = x + szIdx[2];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 3))
    {
        loc.x = x + szIdx[3];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 4))
    {
        loc.x = x + szIdx[4];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 5))
    {
        loc.x = x + szIdx[5];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 6))
    {
        loc.x = x + szIdx[6];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 7))
    {
        loc.x = x + szIdx[7];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }

    if (0 != vgetq_lane_u8(vPred, 8))
    {
        loc.x = x + szIdx[8];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 9))
    {
        loc.x = x + szIdx[9];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 10))
    {
        loc.x = x + szIdx[10];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 11))
    {
        loc.x = x + szIdx[11];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 12))
    {
        loc.x = x + szIdx[12];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 13))
    {
        loc.x = x + szIdx[13];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 14))
    {
        loc.x = x + szIdx[14];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u8(vPred, 15))
    {
        loc.x = x + szIdx[15];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
}

static void addLoc16(vx_uint32 y, vx_uint32 x, uint16x8_t *pvPred, vx_array arrLoc)
{
    uint16x8_t vPred = *pvPred;
    vx_uint32 szIdx[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    vx_coordinates2d_t loc;
    loc.y = y;
    if (0 != vgetq_lane_u16(vPred, 0))
    {
        loc.x = x + szIdx[0];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u16(vPred, 1))
    {
        loc.x = x + szIdx[1];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u16(vPred, 2))
    {
        loc.x = x + szIdx[2];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u16(vPred, 3))
    {
        loc.x = x + szIdx[3];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u16(vPred, 4))
    {
        loc.x = x + szIdx[4];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u16(vPred, 5))
    {
        loc.x = x + szIdx[5];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u16(vPred, 6))
    {
        loc.x = x + szIdx[6];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
    if (0 != vgetq_lane_u16(vPred, 7))
    {
        loc.x = x + szIdx[7];
        vxAddArrayItems(arrLoc, 1, &loc, sizeof(loc));
    }
}

static void calcMinMaxLocu8(vx_uint8 *src_base, vx_imagepatch_addressing_t *src_addr,
                            vx_int64 *pMinVal, vx_int64 *pMaxVal,
                            vx_uint32 *pMinCount, vx_uint32 *pMaxCount,
                            vx_array minLoc, vx_array maxLoc)
{
    vx_uint32 y, x;
    uint8x8_t vMaxVal = vdup_n_u8(0);
    uint8x8_t vMinVal = vdup_n_u8(UINT8_MAX);
    vx_uint8 sMaxVal = 0;
    vx_uint8 sMinVal = UINT8_MAX;
    vx_uint32 w16 = (src_addr->dim_x >> 4) << 4;
    vx_int64 iMinVal = 0;
    vx_int64 iMaxVal = 0;
    vx_uint32 iMinCount = 0;
    vx_uint32 iMaxCount = 0;
    for (y = 0; y < src_addr->dim_y; y++)
    {
        vx_uint8 *ptr_src = src_base + y * src_addr->stride_y;
        for (x = 0; x < w16; x += 16)
        {
            uint8x16_t vSrc = vld1q_u8(ptr_src + x * src_addr->stride_x);
            uint8x8_t vMinTmp = vmin_u8(vget_high_u8(vSrc), vget_low_u8(vSrc));
            uint8x8_t vMaxTmp = vmax_u8(vget_high_u8(vSrc), vget_low_u8(vSrc));

            vMinVal = vmin_u8(vMinVal, vMinTmp);
            vMaxVal = vmax_u8(vMaxVal, vMaxTmp);
        }
        for (x = w16; x < src_addr->dim_x; x++)
        {
            void *src = vxFormatImagePatchAddress2d(src_base, x, y, src_addr);
            sMinVal = MIN(sMinVal, *(vx_uint8 *)src);
            sMaxVal = MAX(sMaxVal, *(vx_uint8 *)src);
        }
    }

    vMinVal = vpmin_u8(vMinVal, vMinVal);
    vMaxVal = vpmax_u8(vMaxVal, vMaxVal);
    vMinVal = vpmin_u8(vMinVal, vMinVal);
    vMaxVal = vpmax_u8(vMaxVal, vMaxVal);
    vMinVal = vpmin_u8(vMinVal, vMinVal);
    vMaxVal = vpmax_u8(vMaxVal, vMaxVal);
    iMinVal = MIN(vget_lane_u8(vMinVal, 0), sMinVal);
    iMaxVal = MAX(vget_lane_u8(vMaxVal, 0), sMaxVal);

    //calculate count and position
    uint8x16_t vMinu8 = vdupq_n_u8((vx_uint8)iMinVal);
    uint8x16_t vMaxu8 = vdupq_n_u8((vx_uint8)iMaxVal);
    uint32x4_t vMinCnt = vdupq_n_u32(0);
    uint32x4_t vMaxCnt = vdupq_n_u32(0);
    uint8x16_t vZero = vdupq_n_u8(0);
    uint8x16_t vOne = vdupq_n_u8(1);
    for (y = 0; y < src_addr->dim_y; y++)
    {
        vx_uint8 *ptr_src = src_base + y * src_addr->stride_y;
        for (x = 0; x < w16; x += 16)
        {
            uint8x16_t vSrc = vld1q_u8(ptr_src + x * src_addr->stride_x);
            uint8x16_t vMinPred = vceqq_u8(vSrc, vMinu8);
            uint8x16_t vMaxPred = vceqq_u8(vSrc, vMaxu8);

            uint8x16_t vCntTmp = vbslq_u8(vMinPred, vOne, vZero);
            uint8x8_t vSumu8 = vadd_u8(vget_low_u8(vCntTmp), vget_high_u8(vCntTmp));
            uint16x8_t vSumu16 = vmovl_u8(vSumu8);
            vMinCnt = vaddq_u32(vMinCnt, vmovl_u16(vget_low_u16(vSumu16)));
            vMinCnt = vaddq_u32(vMinCnt, vmovl_u16(vget_high_u16(vSumu16)));

            vCntTmp = vbslq_u8(vMaxPred, vOne, vZero);
            vSumu8 = vadd_u8(vget_low_u8(vCntTmp), vget_high_u8(vCntTmp));
            vSumu16 = vmovl_u8(vSumu8);
            vMaxCnt = vaddq_u32(vMaxCnt, vmovl_u16(vget_low_u16(vSumu16)));
            vMaxCnt = vaddq_u32(vMaxCnt, vmovl_u16(vget_high_u16(vSumu16)));

            addLocu8(y, x, &vMinPred, minLoc);
            addLocu8(y, x, &vMaxPred, maxLoc);

        }
        for (x = w16; x < src_addr->dim_x; x++)
        {
            void *src = vxFormatImagePatchAddress2d(src_base, x, y, src_addr);
            vx_coordinates2d_t loc;
            loc.y = y;
            loc.x = x;
            if (*(vx_uint8 *)src == iMinVal)
            {
                iMinCount += 1;
                vxAddArrayItems(minLoc, 1, &loc, sizeof(loc));
            }

            if (*(vx_uint8 *)src == iMaxVal)
            {
                iMaxCount += 1;
                vxAddArrayItems(maxLoc, 1, &loc, sizeof(loc));
            }
        }
    }

    uint32x2_t vSumu32 = vadd_u32(vget_low_u32(vMinCnt), vget_high_u32(vMinCnt));
    iMinCount += vget_lane_u32(vSumu32, 0);
    iMinCount += vget_lane_u32(vSumu32, 1);

    vSumu32 = vadd_u32(vget_low_u32(vMaxCnt), vget_high_u32(vMaxCnt));
    iMaxCount += vget_lane_u32(vSumu32, 0);
    iMaxCount += vget_lane_u32(vSumu32, 1);

    *pMinVal = iMinVal;
    *pMaxVal = iMaxVal;
    *pMinCount = iMinCount;
    *pMaxCount = iMaxCount;
}

static void calcMinMaxLocs16(vx_int16 *src_base, vx_imagepatch_addressing_t *src_addr,
                             vx_int64 *pMinVal, vx_int64 *pMaxVal,
                             vx_uint32 *pMinCount, vx_uint32 *pMaxCount,
                             vx_array minLoc, vx_array maxLoc)
{
    vx_uint32 y, x;
    int16x8_t vMaxVal = vdupq_n_s16(INT16_MIN);
    int16x8_t vMinVal = vdupq_n_s16(INT16_MAX);
    vx_int16 sMaxVal = INT16_MIN;
    vx_int16 sMinVal = INT16_MAX;
    vx_uint32 w16 = (src_addr->dim_x >> 4) << 4;
    vx_int64 iMinVal = 0;
    vx_int64 iMaxVal = 0;
    vx_uint32 iMinCount = 0;
    vx_uint32 iMaxCount = 0;
    for (y = 0; y < src_addr->dim_y; y++)
    {
        vx_uint8 *ptr_src = (vx_uint8 *)src_base + y * src_addr->stride_y;
        for (x = 0; x < w16; x += 16)
        {
            int16x8x2_t vSrc = vld2q_s16((vx_int16 *)(ptr_src + x * src_addr->stride_x));
            int16x8_t vMinTmp = vminq_s16(vSrc.val[0], vSrc.val[1]);
            int16x8_t vMaxTmp = vmaxq_s16(vSrc.val[0], vSrc.val[1]);

            vMinVal = vminq_s16(vMinVal, vMinTmp);
            vMaxVal = vmaxq_s16(vMaxVal, vMaxTmp);
        }
        for (x = w16; x < src_addr->dim_x; x++)
        {
            void *src = vxFormatImagePatchAddress2d(src_base, x, y, src_addr);
            sMinVal = MIN(sMinVal, *(vx_int16 *)src);
            sMaxVal = MAX(sMaxVal, *(vx_int16 *)src);
        }
    }

    int16x4_t vMinTmp = vmin_s16(vget_low_s16(vMinVal), vget_high_s16(vMinVal));
    int16x4_t vMaxTmp = vmax_s16(vget_low_s16(vMaxVal), vget_high_s16(vMaxVal));
    vMinTmp = vpmin_s16(vMinTmp, vMinTmp);
    vMaxTmp = vpmax_s16(vMaxTmp, vMaxTmp);
    vMinTmp = vpmin_s16(vMinTmp, vMinTmp);
    vMaxTmp = vpmax_s16(vMaxTmp, vMaxTmp);
    iMinVal = MIN(vget_lane_s16(vMinTmp, 0), sMinVal);
    iMaxVal = MAX(vget_lane_s16(vMaxTmp, 0), sMaxVal);

    //calculate count and position
    int16x8_t vMins16 = vdupq_n_s16((vx_int16)iMinVal);
    int16x8_t vMaxs16 = vdupq_n_s16((vx_int16)iMaxVal);
    uint32x4_t vMinCnt = vdupq_n_u32(0);
    uint32x4_t vMaxCnt = vdupq_n_u32(0);
    int16x8_t vZero = vdupq_n_s16(0);
    int16x8_t vOne = vdupq_n_s16(1);
    vx_uint32 w8 = (src_addr->dim_x >> 3) << 3;
    for (y = 0; y < src_addr->dim_y; y++)
    {
        vx_uint8 *ptr_src = (vx_uint8 *)src_base + y * src_addr->stride_y;
        for (x = 0; x < w8; x += 8)
        {
            int16x8_t vSrc = vld1q_s16((vx_int16 *)(ptr_src + x * src_addr->stride_x));
            uint16x8_t vMinPred = vceqq_s16(vSrc, vMins16);
            uint16x8_t vMaxPred = vceqq_s16(vSrc, vMaxs16);

            int16x8_t vCntTmp = vbslq_s16(vMinPred, vOne, vZero);
            vMinCnt = vaddq_u32(vMinCnt, vmovl_u16(vreinterpret_u16_s16(vget_low_s16(vCntTmp))));
            vMinCnt = vaddq_u32(vMinCnt, vmovl_u16(vreinterpret_u16_s16(vget_high_s16(vCntTmp))));

            vCntTmp = vbslq_s16(vMaxPred, vOne, vZero);
            vMaxCnt = vaddq_u32(vMaxCnt, vmovl_u16(vreinterpret_u16_s16(vget_low_s16(vCntTmp))));
            vMaxCnt = vaddq_u32(vMaxCnt, vmovl_u16(vreinterpret_u16_s16(vget_high_s16(vCntTmp))));

            addLoc16(y, x, &vMinPred, minLoc);
            addLoc16(y, x, &vMaxPred, maxLoc);

        }
        for (x = w8; x < src_addr->dim_x; x++)
        {
            void *src = vxFormatImagePatchAddress2d(src_base, x, y, src_addr);
            vx_coordinates2d_t loc;
            loc.y = y;
            loc.x = x;
            if (*(vx_int16 *)src == iMinVal)
            {
                iMinCount += 1;
                vxAddArrayItems(minLoc, 1, &loc, sizeof(loc));
            }

            if (*(vx_int16 *)src == iMaxVal)
            {
                iMaxCount += 1;
                vxAddArrayItems(maxLoc, 1, &loc, sizeof(loc));
            }
        }
    }

    uint32x2_t vSumu32 = vadd_u32(vget_low_u32(vMinCnt), vget_high_u32(vMinCnt));
    iMinCount += vget_lane_u32(vSumu32, 0);
    iMinCount += vget_lane_u32(vSumu32, 1);

    vSumu32 = vadd_u32(vget_low_u32(vMaxCnt), vget_high_u32(vMaxCnt));
    iMaxCount += vget_lane_u32(vSumu32, 0);
    iMaxCount += vget_lane_u32(vSumu32, 1);

    *pMinVal = iMinVal;
    *pMaxVal = iMaxVal;
    *pMinCount = iMinCount;
    *pMaxCount = iMaxCount;
}

vx_status vxMinMaxLoc(vx_image input, vx_scalar minVal, vx_scalar maxVal, vx_array minLoc, vx_array maxLoc, vx_scalar minCount, vx_scalar maxCount)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t rect;
    vx_df_image format;
    vx_int64 iMinVal = INT64_MAX;
    vx_int64 iMaxVal = INT64_MIN;
    vx_uint32 iMinCount = 0;
    vx_uint32 iMaxCount = 0;
    vx_map_id map_id = 0;
    vx_status status = VX_SUCCESS;

    status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(input, &rect);
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &src_addr, (void **)&src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

    vxTruncateArray(minLoc, 0);
    vxTruncateArray(maxLoc, 0);

    if (format == VX_DF_IMAGE_U8)
    {
        calcMinMaxLocu8((vx_uint8 *)src_base, &src_addr, &iMinVal, &iMaxVal,
                        &iMinCount, &iMaxCount, minLoc, maxLoc);
    }
    else if (format == VX_DF_IMAGE_S16)
    {
        calcMinMaxLocs16((vx_int16 *)src_base, &src_addr, &iMinVal, &iMaxVal,
                         &iMinCount, &iMaxCount, minLoc, maxLoc);
    }

    status |= vxUnmapImagePatch(input, map_id);

    switch (format)
    {
        case VX_DF_IMAGE_U8:
            {
                vx_uint8 min = (vx_uint8)iMinVal, max = (vx_uint8)iMaxVal;
                if (minVal) vxCopyScalar(minVal, &min, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                if (maxVal) vxCopyScalar(maxVal, &max, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            }
            break;
        case VX_DF_IMAGE_U16:
            {
                vx_uint16 min = (vx_uint16)iMinVal, max = (vx_uint16)iMaxVal;
                if (minVal) vxCopyScalar(minVal, &min, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                if (maxVal) vxCopyScalar(maxVal, &max, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            }
            break;
        case VX_DF_IMAGE_U32:
            {
                vx_uint32 min = (vx_uint32)iMinVal, max = (vx_uint32)iMaxVal;
                if (minVal) vxCopyScalar(minVal, &min, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                if (maxVal) vxCopyScalar(maxVal, &max, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            }
            break;
        case VX_DF_IMAGE_S16:
            {
                vx_int16 min = (vx_int16)iMinVal, max = (vx_int16)iMaxVal;
                if (minVal) vxCopyScalar(minVal, &min, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                if (maxVal) vxCopyScalar(maxVal, &max, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            }
            break;
        case VX_DF_IMAGE_S32:
            {
                vx_int32 min = (vx_int32)iMinVal, max = (vx_int32)iMaxVal;
                if (minVal) vxCopyScalar(minVal, &min, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
                if (maxVal) vxCopyScalar(maxVal, &max, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            }
            break;
    }

    if (minCount) vxCopyScalar(minCount, &iMinCount, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    if (maxCount) vxCopyScalar(maxCount, &iMaxCount, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    return status;
}

