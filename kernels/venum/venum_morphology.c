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
#include <stdio.h>

static inline void opt_max(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t max = vmax_u8(*a, *b);
    *a = max;
}
static inline void opt_min(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t min = vmin_u8(*a, *b);
    *a = min;
}

static vx_uint8 min_op(vx_uint8 a, vx_uint8 b) 
{
    return a < b ? a : b;
}

static vx_uint8 max_op(vx_uint8 a, vx_uint8 b) 
{
    return a > b ? a : b;
}

// helpers
static vx_int32 vx_uint8_compare(const void *p1, const void *p2)
{
    vx_uint8 a = *(vx_uint8 *)p1;
    vx_uint8 b = *(vx_uint8 *)p2;
    if (a > b)
        return 1;
    else if (a == b)
        return 0;
    else
        return -1;
}

static vx_uint32 readMaskedRectangle_U8(const void *base,
    const vx_imagepatch_addressing_t *addr,
    const vx_border_t *borders,
    vx_df_image type,
    vx_uint32 center_x,
    vx_uint32 center_y,
    vx_uint32 left,
    vx_uint32 top,
    vx_uint32 right,
    vx_uint32 bottom,
    vx_uint8 *mask,
    vx_uint8 *destination)
{
    vx_int32 width = (vx_int32)addr->dim_x, height = (vx_int32)addr->dim_y;
    vx_int32 stride_y = addr->stride_y;
    vx_int32 stride_x = addr->stride_x;
    const vx_uint8 *ptr = (const vx_uint8 *)base;
    vx_int32 ky, kx;
    vx_uint32 mask_index = 0;
    vx_uint32 dest_index = 0;
    // kx, kx - kernel x and y
    if (borders->mode == VX_BORDER_REPLICATE || borders->mode == VX_BORDER_UNDEFINED)
    {
        for (ky = -(vx_int32)top; ky <= (vx_int32)bottom; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            y = y < 0 ? 0 : y >= height ? height - 1 : y;

            for (kx = -(vx_int32)left; kx <= (vx_int32)right; ++kx, ++mask_index)
            {
                vx_int32 x = (vx_int32)(center_x + kx);
                x = x < 0 ? 0 : x >= width ? width - 1 : x;
                if (mask[mask_index])
                    ((vx_uint8*)destination)[dest_index++] = *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
            }
        }
    }
    else if (borders->mode == VX_BORDER_CONSTANT)
    {
        vx_pixel_value_t cval = borders->constant_value;
        for (ky = -(vx_int32)top; ky <= (vx_int32)bottom; ++ky)
        {
            vx_int32 y = (vx_int32)(center_y + ky);
            vx_int32 ccase_y = y < 0 || y >= height;

            for (kx = -(vx_int32)left; kx <= (vx_int32)right; ++kx, ++mask_index)
            {
                vx_int32 x = (vx_int32)(center_x + kx);
                vx_int32 ccase = ccase_y || x < 0 || x >= width;
                if (mask[mask_index])
                    ((vx_uint8*)destination)[dest_index++] = ccase ? (vx_uint8)cval.U8 : *(vx_uint8*)(ptr + y*stride_y + x*stride_x);
            }
        }
    }

    return dest_index;
}

// nodeless version of the Erode3x3 kernel
vx_status vxErode3x3(vx_image src, vx_image dst, vx_border_t *borders)
{
    vx_uint32 y, x, low_y = 0, low_x = 0, high_y, high_x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;

    vx_status status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    high_x = src_addr.dim_x;
    high_y = src_addr.dim_y;

    ++low_x; --high_x;
    ++low_y; --high_y;
    vxAlterRectangle(&rect, 1, 1, -1, -1);

    vx_uint32 width_x8 = high_x / 8 * 8;

    vx_uint8 pixels[9], m;
    vx_uint32 i;

    for (y = low_y; (y < high_y) && (status == VX_SUCCESS); y++)
    {
        vx_uint8 *dst = (vx_uint8 *)dst_base + low_x + y * src_addr.dim_x;
        vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * src_addr.dim_x;
        vx_uint8* mid_src = (vx_uint8 *)src_base + (y)* src_addr.dim_x;
        vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1)* src_addr.dim_x;

        for (x = low_x; x < width_x8; x+=8)
        {
            const uint8x16_t top_data = vld1q_u8(top_src);
            const uint8x16_t mid_data = vld1q_u8(mid_src);
            const uint8x16_t bot_data = vld1q_u8(bot_src);
            
            uint8x8_t p0 = vget_low_u8(top_data);
            uint8x8_t p1 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 1);
            uint8x8_t p2 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 2);
            uint8x8_t p3 = vget_low_u8(mid_data);
            uint8x8_t p4 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 1);
            uint8x8_t p5 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 2);
            uint8x8_t p6 = vget_low_u8(bot_data);
            uint8x8_t p7 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 1);
            uint8x8_t p8 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 2);

            opt_min(&p0, &p1);
            opt_min(&p0, &p2);
            opt_min(&p0, &p3);
            opt_min(&p0, &p4);
            opt_min(&p0, &p5);
            opt_min(&p0, &p6);
            opt_min(&p0, &p7);
            opt_min(&p0, &p8);

            vst1_u8(dst, p0);       
            
            top_src+=8;
            mid_src+=8;
            bot_src+=8;
            dst += 8;
        }
        for (x = low_x; x < high_x; x++)
        {
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = min_op(m, pixels[i]);

            *dst = m;
        }
    }

    if (borders->mode == VX_BORDER_REPLICATE || borders->mode == VX_BORDER_CONSTANT)
    {
        ++high_x;
        ++high_y;

        //Calculate border
        for (y = 0; y < high_y; y++)
        {
            x = 0;
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = min_op(m, pixels[i]);

            *dst = m;


            x = high_x - 1;
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = min_op(m, pixels[i]);

            *dst = m;
        }

        for (x = 1; x < high_x - 1; x++)
        {
            y = 0;
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = min_op(m, pixels[i]);

            *dst = m;

            y = high_y - 1;
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = min_op(m, pixels[i]);

            *dst = m;
        }
    }

    status |= vxUnmapImagePatch(src, map_id);
    status |= vxUnmapImagePatch(dst, result_map_id);
    
    return status;
}

// nodeless version of the Dilate3x3 kernel
vx_status vxDilate3x3(vx_image src, vx_image dst, vx_border_t *borders)
{
    vx_uint32 y, x, low_y = 0, low_x = 0, high_y, high_x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;

    vx_status status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    high_x = src_addr.dim_x;
    high_y = src_addr.dim_y;

    ++low_x; --high_x;
    ++low_y; --high_y;
    vxAlterRectangle(&rect, 1, 1, -1, -1);

    vx_uint32 width_x8 = high_x / 8 * 8;

    vx_uint8 pixels[9], m;
    vx_uint32 i;

    for (y = low_y; (y < high_y) && (status == VX_SUCCESS); y++)
    {
        vx_uint8* dst = (vx_uint8 *)dst_base + low_x + y * src_addr.dim_x;
        vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * src_addr.dim_x;
        vx_uint8* mid_src = (vx_uint8 *)src_base + (y)* src_addr.dim_x;
        vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1)* src_addr.dim_x;
        for (x = low_x; x < width_x8; x+=8)
        {
            const uint8x16_t top_data = vld1q_u8(top_src);
            const uint8x16_t mid_data = vld1q_u8(mid_src);
            const uint8x16_t bot_data = vld1q_u8(bot_src);
            
            uint8x8_t p0 = vget_low_u8(top_data);
            uint8x8_t p1 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 1);
            uint8x8_t p2 = vext_u8(vget_low_u8(top_data), vget_high_u8(top_data), 2);
            uint8x8_t p3 = vget_low_u8(mid_data);
            uint8x8_t p4 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 1);
            uint8x8_t p5 = vext_u8(vget_low_u8(mid_data), vget_high_u8(mid_data), 2);
            uint8x8_t p6 = vget_low_u8(bot_data);
            uint8x8_t p7 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 1);
            uint8x8_t p8 = vext_u8(vget_low_u8(bot_data), vget_high_u8(bot_data), 2);

            opt_max(&p0, &p1);
            opt_max(&p0, &p2);
            opt_max(&p0, &p3);
            opt_max(&p0, &p4);
            opt_max(&p0, &p5);
            opt_max(&p0, &p6);
            opt_max(&p0, &p7);
            opt_max(&p0, &p8);

            vst1_u8(dst, p0);

            top_src+=8;
            mid_src+=8;
            bot_src+=8;
            dst += 8;
        }
        for (x = low_x; x < high_x; x++)
        {
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = max_op(m, pixels[i]);

            *dst = m;
        }
    }

    if (borders->mode == VX_BORDER_REPLICATE || borders->mode == VX_BORDER_CONSTANT)
    {
        ++high_x;
        ++high_y;

        //Calculate border
        for (y = 0; y < high_y; y++)
        {
            x = 0;
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = max_op(m, pixels[i]);

            *dst = m;


            x = high_x - 1;
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = max_op(m, pixels[i]);

            *dst = m;
        }

        for (x = 1; x < high_x - 1; x++)
        {
            y = 0;
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = max_op(m, pixels[i]);

            *dst = m;

            y = high_y - 1;
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels);

            m = pixels[0];
            for (i = 1; i < dimof(pixels); i++)
                m = max_op(m, pixels[i]);

            *dst = m;
        }
    }

    status |= vxUnmapImagePatch(src, map_id);
    status |= vxUnmapImagePatch(dst, result_map_id);
    
    return status;
}