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

// nodeless version of the Histogram kernel
vx_status vxHistogram(vx_image src, vx_distribution dist)
{
    vx_rectangle_t src_rect;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    void* src_base = NULL;
    void* dist_ptr = NULL;
    vx_df_image format = 0;
    vx_uint32 x = 0;
    vx_uint32 y = 0;
    vx_int32 offset = 0;
    vx_uint32 range = 0;
    vx_size numBins = 0;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_status status = VX_SUCCESS;

    vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
    vxQueryDistribution(dist, VX_DISTRIBUTION_BINS, &numBins, sizeof(numBins));
    vxQueryDistribution(dist, VX_DISTRIBUTION_RANGE, &range, sizeof(range));
    vxQueryDistribution(dist, VX_DISTRIBUTION_OFFSET, &offset, sizeof(offset));

    status = vxGetValidRegionImage(src, &src_rect);
    status |= vxMapImagePatch(src, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapDistribution(dist, &dst_map_id, &dist_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    if (status == VX_SUCCESS)
    {
        vx_int32 *dist_tmp = dist_ptr;
        for (x = 0; x < numBins; x++)
        {
            dist_tmp[x] = 0;
        }
        
        vx_int32 roiw8 = src_addr.dim_x >= 7 ? src_addr.dim_x - 7 : 0;
        uint32x4_t offset32x4 = vdupq_n_u32((vx_uint32)offset);
        vx_int32 tmpint32 = offset+range;
        uint32x4_t offset_range32x4 = vdupq_n_u32((vx_uint32)tmpint32);
        for (y = 0; y < src_addr.dim_y; y++)
        {
            vx_uint8 * src_y_ptr_u8 = (vx_uint8 *)src_base + src_addr.stride_y * y;
            vx_uint16 * src_y_ptr_u16 = (vx_uint16 *)src_base + src_addr.stride_y * y;
            for (x = 0; x < roiw8; x+=8)
            {                
                if (format == VX_DF_IMAGE_U8)
                {
                    vx_uint8 *src_ptr_u8 = src_y_ptr_u8 + src_addr.stride_x * x;
                    uint8x8_t pixel8x8 = vld1_u8(src_ptr_u8);
                    
                    uint16x8_t tmp16x8 = vmovl_u8 (pixel8x8);
                    uint32x4x2_t pixel32x4 =
                    {
                        {
                            vmovl_u16 (vget_low_u16(tmp16x8)),
                            vmovl_u16 (vget_high_u16(tmp16x8))
                        }
                    };
                    uint32x4_t logical_lq_32x4_0 =  vcleq_u32(offset32x4, pixel32x4.val[0]); 
                    uint32x4_t logical_lq_32x4_1 =  vcleq_u32(offset32x4, pixel32x4.val[1]); 
                    uint32x4_t logical_l_32x4_0 =  vcltq_u32(pixel32x4.val[0], offset_range32x4);
                    uint32x4_t logical_l_32x4_1 =  vcltq_u32(pixel32x4.val[1], offset_range32x4);
                    uint32x4_t logical_and_32x4_0 = vandq_u32(logical_lq_32x4_0, logical_l_32x4_0);
                    uint32x4_t logical_and_32x4_1 = vandq_u32(logical_lq_32x4_1, logical_l_32x4_1);
                    if(vgetq_lane_u32(logical_and_32x4_0, 0) == 0xFFFFFFFF)// 0xFFFFFF
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 0) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_0, 1) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 1) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_0, 2) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 2) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_0, 3) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 3) -  (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 0) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 0) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 1) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 1) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 2) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 2) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 3) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 3) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                }
                else if (format == VX_DF_IMAGE_U16)
                {
                    vx_uint16 *src_ptr_u16 = src_y_ptr_u16 + src_addr.stride_x * x;
                    uint16x8_t pixel16x8 = vld1q_u16(src_ptr_u16);
                    uint32x4x2_t pixel32x4 =
                    {
                        {
                            vmovl_u16 (vget_low_u16(pixel16x8)),
                            vmovl_u16 (vget_high_u16(pixel16x8))
                        }
                    };
                    uint32x4_t logical_lq_32x4_0 =  vcleq_u32(offset32x4, pixel32x4.val[0]); 
                    uint32x4_t logical_lq_32x4_1 =  vcleq_u32(offset32x4, pixel32x4.val[1]); 
                    uint32x4_t logical_l_32x4_0 =  vcltq_u32(pixel32x4.val[0], offset_range32x4);
                    uint32x4_t logical_l_32x4_1 =  vcltq_u32(pixel32x4.val[1], offset_range32x4);
                    uint32x4_t logical_and_32x4_0 = vandq_u32(logical_lq_32x4_0, logical_l_32x4_0);
                    uint32x4_t logical_and_32x4_1 = vandq_u32(logical_lq_32x4_1, logical_l_32x4_1);
                    
                    if(vgetq_lane_u32(logical_and_32x4_0, 0) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 0) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_0, 1) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 1) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_0, 2) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 2) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_0, 3) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[0], 3) -  (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 0) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 0) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 1) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 1) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 2) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 2) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                    if(vgetq_lane_u32(logical_and_32x4_1, 3) == 0xFFFFFFFF)
                    {
                        vx_size index = (vgetq_lane_u32(pixel32x4.val[1], 3) - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                }
            }
            for (; x < src_addr.dim_x; x++)
            {
                if (format == VX_DF_IMAGE_U8)
                {
                    vx_uint8 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                    vx_uint8 pixel = *src_ptr;
                    if (((vx_size)offset <= (vx_size)pixel) && ((vx_size)pixel < (vx_size)(offset+range)))
                    {
                        vx_size index = (pixel - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                }
                else if (format == VX_DF_IMAGE_U16)
                {
                    vx_uint16 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                    vx_uint16 pixel = *src_ptr;
                    if (((vx_size)offset <= (vx_size)pixel) && ((vx_size)pixel < (vx_size)(offset+range)))
                    {
                        vx_size index = (pixel - (vx_uint16)offset) * numBins / range;
                        dist_tmp[index]++;
                    }
                }
            }
        }
    }
    status |= vxUnmapDistribution(dist, dst_map_id);
    status |= vxUnmapImagePatch(src, src_map_id);
    return status;
}

// nodeless version of the EqualizeHist kernel
vx_status vxEqualizeHist(vx_image src, vx_image dst)
{
    vx_uint32 x;
    vx_uint32 y;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t rect;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(src, &rect);

    status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    if (status == VX_SUCCESS)
    {
        /* for 16-bit support (U16 or S16), the code can be duplicated with NUM_BINS = 65536 and PIXEL = vx_uint16. */
        #define NUM_BINS 256

        /* allocate a fixed-size temp array to store the image histogram & cumulative distribution */
        vx_uint32 hist[NUM_BINS] = {0};
        vx_uint8 eqHist[NUM_BINS] = {0};
        vx_uint32 cdf[NUM_BINS] = {0};
        vx_uint32 sum = 0, div;
        vx_uint8 minv = 0xFF;

        /* calculate the distribution (histogram) */
        for (y = 0; y < src_addr.dim_y; y++)
        {
            const vx_uint8 * src_ptr = (vx_uint8 *)src_base + y * src_addr.stride_y;
  
            for (x = 0; x < src_addr.dim_x - 8; x += 8)
            {
                const uint8x8_t pixels = vld1_u8(src_ptr + x);

                ++hist[vget_lane_u8(pixels, 0)];
                ++hist[vget_lane_u8(pixels, 1)];
                ++hist[vget_lane_u8(pixels, 2)];
                ++hist[vget_lane_u8(pixels, 3)];
                ++hist[vget_lane_u8(pixels, 4)];
                ++hist[vget_lane_u8(pixels, 5)];
                ++hist[vget_lane_u8(pixels, 6)];
                ++hist[vget_lane_u8(pixels, 7)];
            }

            for (; x < src_addr.dim_x; ++x)
                ++hist[src_ptr[x]];
        }

        for (x = 0; x < NUM_BINS; x++)
        {
            sum += hist[x];
            cdf[x] = sum;
        }
        div = (src_addr.dim_x * src_addr.dim_y) - cdf[minv];
        if (div > 0)
        {           
            float32x4_t vDiv = vdupq_n_f32(1 / (vx_float32)div);
            float32x4_t vMinv = vdupq_n_f32((vx_float32)cdf[minv]);
            float32x4_t v_05 = vdupq_n_f32(0.5);
            float32x4_t v_255 = vdupq_n_f32(255.0);
            vx_float32 mid[4];
            for (x = 0; x < NUM_BINS / 4; x++)
            {
                vx_uint8 * histCurr = eqHist + x * 4;
                vx_uint32 * pCdf = cdf + x * 4;
                float32_t curr4[4] = { (vx_float32)*pCdf, (vx_float32)*(pCdf+1), (vx_float32)*(pCdf+2), (vx_float32)*(pCdf+3) };
                float32x4_t vCdf = vld1q_f32(curr4);
                float32x4_t vTmp = vsubq_f32(vCdf, vMinv);
                vTmp = vmulq_f32(vTmp, vDiv);
                vTmp = vmlaq_f32(v_05, vTmp, v_255);
                vst1q_f32(mid, vTmp);
                *histCurr = (vx_uint8)mid[0];
                *(histCurr + 1) = (vx_uint8)mid[1];
                *(histCurr + 2) = (vx_uint8)mid[2];
                *(histCurr + 3) = (vx_uint8)mid[3];
            }
        }
        else
        {
            vx_uint8 num[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
            uint8x16_t v_16 = vdupq_n_u8(16);
            uint8x16_t vNum = vld1q_u8(num);
            for (x = 0; x < NUM_BINS / 16; x++)
            {
                vx_uint8 * histCurr = eqHist + x * 16;
                vst1q_u8(histCurr, vNum);
                vNum = vaddq_u8(vNum, v_16);
            }
        }

        /* map the src pixel values to the equalized pixel values */
        for (y = 0; y < src_addr.dim_y; y++)
        {
            const vx_uint8 * src_ptr = (vx_uint8 *)src_base + y * src_addr.stride_y;
            vx_uint8 * dst_ptr = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            vx_int32 roiw8 = src_addr.dim_x >= 7 ? src_addr.dim_x - 7 : 0;
            x = 0;
            for (; x < roiw8; x += 8)
            {
                dst_ptr[x] = eqHist[src_ptr[x]];
                dst_ptr[x + 1] = eqHist[src_ptr[x + 1]];
                dst_ptr[x + 2] = eqHist[src_ptr[x + 2]];
                dst_ptr[x + 3] = eqHist[src_ptr[x + 3]];
                dst_ptr[x + 4] = eqHist[src_ptr[x + 4]];
                dst_ptr[x + 5] = eqHist[src_ptr[x + 5]];
                dst_ptr[x + 6] = eqHist[src_ptr[x + 6]];
                dst_ptr[x + 7] = eqHist[src_ptr[x + 7]];
            }
            for (; x < src_addr.dim_x; x++)
            {
                dst_ptr[x] = eqHist[src_ptr[x]];
            }
        }
    }

    status |= vxUnmapImagePatch(src, src_map_id);
    status |= vxUnmapImagePatch(dst, dst_map_id);

    return status;
}

