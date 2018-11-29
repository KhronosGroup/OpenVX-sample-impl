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

#include <c_model.h>
#include <stdio.h>

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

    //printf("distribution:%p bins:%u off:%u range:%u\n", dist_ptr, numBins, offset, range);
    if (status == VX_SUCCESS)
    {
        vx_int32 *dist_tmp = dist_ptr;

        /* clear the distribution */
        for (x = 0; x < numBins; x++)
        {
            dist_tmp[x] = 0;
        }

        for (y = 0; y < src_addr.dim_y; y++)
        {
            for (x = 0; x < src_addr.dim_x; x++)
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
        vx_uint32 cdf[NUM_BINS] = {0};
        vx_uint32 sum = 0, div;
        vx_uint8 minv = 0xFF;

        /* calculate the distribution (histogram) */
        for (y = 0; y < src_addr.dim_y; y++)
        {
            for (x = 0; x < src_addr.dim_x; x++)
            {
                vx_uint8 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint8 pixel = (*src_ptr);
                hist[pixel]++;
                if (minv > pixel)
                    minv = pixel;
            }
        }
        /* calculate the cumulative distribution (summed histogram) */
        for (x = 0; x < NUM_BINS; x++)
        {
            sum += hist[x];
            cdf[x] = sum;
        }
        div = (src_addr.dim_x * src_addr.dim_y) - cdf[minv];
        if( div > 0 )
        {
            /* recompute the histogram to be a LUT for replacing pixel values */
            for (x = 0; x < NUM_BINS; x++)
            {
                uint32_t cdfx = cdf[x] - cdf[minv];
                vx_float32 p = (vx_float32)cdfx/(vx_float32)div;
                hist[x] = (uint8_t)(p * 255.0f + 0.5f);
            }
        }
        else
        {
            for (x = 0; x < NUM_BINS; x++)
            {
                hist[x] = x;
            }
        }

        /* map the src pixel values to the equalized pixel values */
        for (y = 0; y < src_addr.dim_y; y++)
        {
            for (x = 0; x < src_addr.dim_x; x++)
            {
                vx_uint8 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst_ptr = hist[(*src_ptr)];
            }
        }
    }

    status |= vxUnmapImagePatch(src, src_map_id);
    status |= vxUnmapImagePatch(dst, dst_map_id);

    return status;
}

