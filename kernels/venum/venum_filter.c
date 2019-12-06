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
#include <stdlib.h>
#include <arm_neon.h>
#include <string.h>
#include <stdio.h>

static vx_uint8 vx_clamp_u8_i32(vx_int32 value)
{
    vx_uint8 v = 0;
    if (value > 255)
    {
        v = 255;
    }
    else if (value < 0)
    {
        v = 0;
    }
    else
    {
        v = (vx_uint8)value;
    }
    return v;
}

static vx_int16 vx_clamp_s16_i32(vx_int32 value)
{
    vx_int16 v = 0;
    if (value > INT16_MAX)
    {
        v = INT16_MAX;
    }
    else if (value < INT16_MIN)
    {
        v = INT16_MIN;
    }
    else
    {
        v = (vx_int16)value;
    }
    return v;
}

static vx_int32 vx_convolve8with16(void *base, vx_uint32 x, vx_uint32 y, vx_imagepatch_addressing_t *addr,
                                   vx_int16 conv[3][3], const vx_border_t *borders)
{
    vx_uint8 pixels[3][3];
    vx_int32 div, sum;
    vxReadRectangle(base, addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, &pixels[0][0], 0);
    div = conv[0][0] + conv[0][1] + conv[0][2] +
          conv[1][0] + conv[1][1] + conv[1][2] +
          conv[2][0] + conv[2][1] + conv[2][2];
    sum = (conv[0][0] * pixels[0][0]) + (conv[0][1] * pixels[0][1]) + (conv[0][2] * pixels[0][2]) +
          (conv[1][0] * pixels[1][0]) + (conv[1][1] * pixels[1][1]) + (conv[1][2] * pixels[1][2]) +
          (conv[2][0] * pixels[2][0]) + (conv[2][1] * pixels[2][1]) + (conv[2][2] * pixels[2][2]);
    if (div == 0)
        div = 1;
    return sum / div;
}

static inline void sort(uint8x8_t *a, uint8x8_t *b)
{
    const uint8x8_t min = vmin_u8(*a, *b);
    const uint8x8_t max = vmax_u8(*a, *b);
    *a = min;
    *b = max;
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

// nodeless version of the Median3x3 kernel
vx_status vxMedian3x3_U1(vx_image src, vx_image dst, vx_border_t *borders)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_df_image format = 0;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y, shift_x_u1;

    vx_status status = vxGetValidRegionImage(src, &rect);
    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    shift_x_u1 = (format == VX_DF_IMAGE_U1) ? rect.start_x % 8 : 0;
    high_x = src_addr.dim_x - shift_x_u1;   // U1 addressing rounds down imagepatch start_x to nearest byte boundary
    high_y = src_addr.dim_y;

    if (borders->mode == VX_BORDER_UNDEFINED)
    {
        ++low_x; --high_x;
        ++low_y; --high_y;
        vxAlterRectangle(&rect, 1, 1, -1, -1);
    }

    for (y = low_y; (y < high_y) && (status == VX_SUCCESS); y++)
    {
        for (x = low_x; x < high_x; x++)
        {
            vx_uint32 xShftd = x + shift_x_u1;      // Bit-shift for U1 valid region start
            vx_uint8 *dst_ptr = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xShftd, y, &dst_addr);
            vx_uint8 values[9];

            vxReadRectangle(src_base, &src_addr, borders, format, xShftd, y, 1, 1, values, shift_x_u1);

            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
            /* pick the middle value */
            if (format == VX_DF_IMAGE_U1)
                *dst_ptr = (*dst_ptr & ~(1 << (xShftd % 8))) | (values[4] << (xShftd % 8));
            else
                *dst_ptr = values[4];
        }
    }

    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

    return status;
}


// nodeless version of the Median3x3 kernel
vx_status vxMedian3x3_U8(vx_image src, vx_image dst, vx_border_t *borders)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y;

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

    vx_uint8 values[9], m;
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

            sort(&p1, &p2);
            sort(&p4, &p5);
            sort(&p7, &p8);

            sort(&p0, &p1);
            sort(&p3, &p4);
            sort(&p6, &p7);

            sort(&p1, &p2);
            sort(&p4, &p5);
            sort(&p7, &p8);

            sort(&p0, &p3);
            sort(&p5, &p8);
            sort(&p4, &p7);

            sort(&p3, &p6);
            sort(&p1, &p4);
            sort(&p2, &p5);

            sort(&p4, &p7);
            sort(&p4, &p2);
            sort(&p6, &p4);

            sort(&p4, &p2);

            vst1_u8(dst, p4);

            top_src+=8;
            mid_src+=8;
            bot_src+=8;
            dst += 8;
        }
        for (x = low_x; x < high_x; x++)
        {
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, values, 0);

            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
            *dst = values[4]; /* pick the middle value */
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

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, values, 0);

            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
            *dst = values[4]; /* pick the middle value */


            x = high_x - 1;
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, values, 0);

            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
            *dst = values[4]; /* pick the middle value */
        }

        for (x = 1; x < high_x - 1; x++)
        {
            y = 0;
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, values, 0);

            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
            *dst = values[4]; /* pick the middle value */

            y = high_y - 1;
            dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            vxReadRectangle(src_base, &src_addr, borders, VX_DF_IMAGE_U8, x, y, 1, 1, values, 0);

            qsort(values, dimof(values), sizeof(vx_uint8), vx_uint8_compare);
            *dst = values[4]; /* pick the middle value */
        }
    }

    status |= vxUnmapImagePatch(src, map_id);
    status |= vxUnmapImagePatch(dst, result_map_id);

    return status;
}

vx_status vxMedian3x3(vx_image src, vx_image dst, vx_border_t *borders)
{
    vx_df_image format;
    vx_status status = vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1)
        return vxMedian3x3_U1(src, dst, borders);
    else
        return vxMedian3x3_U8(src, dst, borders);
}


// nodeless version of the Box3x3 kernel
static vx_int16 box[3][3] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1},
};

vx_status vxBox3x3(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_enum dst_format = VX_DF_IMAGE_VIRT;
    vx_status status = VX_SUCCESS;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y;

    status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

    high_x = src_addr.dim_x;
    high_y = src_addr.dim_y;

    vx_int32 src_stride_y = src_addr.stride_y;
    vx_int32 dst_stride_y = dst_addr.stride_y;

    // For box3x3 conv
    const float32x4_t oneovernine = vdupq_n_f32(1.0f / 9.0f);

    ++low_x; --high_x;
    ++low_y; --high_y;
    vxAlterRectangle(&rect, 1, 1, -1, -1);

    vx_uint32 width_x8 = high_x / 8 * 8;

    for (y = low_y; y < high_y; y++)
    {
        vx_uint8* dst_u8 = (vx_uint8 *)dst_base + low_x + y * dst_stride_y;
        vx_int16* dst_s16 = (vx_int16 *)dst_base + low_x + y * dst_stride_y;
        vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * src_stride_y;
        vx_uint8* mid_src = (vx_uint8 *)src_base + (y)* src_stride_y;
        vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1)* src_stride_y;

        for (x = low_x; x < width_x8; x += 8)
        {
            const uint8x16_t top_data = vld1q_u8(top_src);
            const uint8x16_t mid_data = vld1q_u8(mid_src);
            const uint8x16_t bot_data = vld1q_u8(bot_src);

            const int16x8x2_t top_s16 =
            {
                {
                    vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(top_data))),
                    vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(top_data)))
                }
            };
            const int16x8x2_t mid_s16 =
            {
                {
                    vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(mid_data))),
                    vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(mid_data)))
                }
            };
            const int16x8x2_t bot_s16 =
            {
                {
                    vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(bot_data))),
                    vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(bot_data)))
                }
            };

            //top left
            int16x8_t out = top_s16.val[0];
            //top mid
            out = vaddq_s16(out, vextq_s16(top_s16.val[0], top_s16.val[1], 1));
            //top right
            out = vaddq_s16(out, vextq_s16(top_s16.val[0], top_s16.val[1], 2));
            //mid left
            out = vaddq_s16(out, mid_s16.val[0]);
            //mid mid
            out = vaddq_s16(out, vextq_s16(mid_s16.val[0], mid_s16.val[1], 1));
            //mid right
            out = vaddq_s16(out, vextq_s16(mid_s16.val[0], mid_s16.val[1], 2));
            //bot left
            out = vaddq_s16(out, bot_s16.val[0]);
            //bot mid
            out = vaddq_s16(out, vextq_s16(bot_s16.val[0], bot_s16.val[1], 1));
            //bot right
            out = vaddq_s16(out, vextq_s16(bot_s16.val[0], bot_s16.val[1], 2));

            float32x4_t outfloathigh = vcvtq_f32_s32(vmovl_s16(vget_high_s16(out)));
            float32x4_t outfloatlow  = vcvtq_f32_s32(vmovl_s16(vget_low_s16(out)));

            outfloathigh = vmulq_f32(outfloathigh, oneovernine);
            outfloatlow  = vmulq_f32(outfloatlow, oneovernine);

            out = vcombine_s16(vqmovn_s32(vcvtq_s32_f32(outfloatlow)),
                               vqmovn_s32(vcvtq_s32_f32(outfloathigh)));

            if (dst_format == VX_DF_IMAGE_U8)
                vst1_u8(dst_u8, vqmovun_s16(out));
            else
                vst1q_s16(dst_s16, out);


            top_src += 8;
            mid_src += 8;
            bot_src += 8;
            dst_u8 += 8;
            dst_s16 += 8;
        }

        for (; x < high_x; x++)
        {
            vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, box, bordermode);

            if (dst_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_u8_i32(value);
            }
            else
            {
                vx_int16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_s16_i32(value);
            }
        }
    }

    if (bordermode->mode == VX_BORDER_REPLICATE || bordermode->mode == VX_BORDER_CONSTANT)
    {
        ++high_x;
        ++high_y;

        //Calculate border
        for (y = 0; y < high_y; y++)
        {
            x = 0;
            vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, box, bordermode);

            if (dst_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_u8_i32(value);
            }
            else
            {
                vx_int16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_s16_i32(value);
            }

            x = high_x - 1;
            value = vx_convolve8with16(src_base, x, y, &src_addr, box, bordermode);

            if (dst_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_u8_i32(value);
            }
            else
            {
                vx_int16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_s16_i32(value);
            }
        }

        for (x = 1; x < high_x - 1; x++)
        {
            y = 0;
            vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, box, bordermode);

            if (dst_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_u8_i32(value);
            }
            else
            {
                vx_int16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_s16_i32(value);
            }

            y = high_y - 1;
            value = vx_convolve8with16(src_base, x, y, &src_addr, box, bordermode);

            if (dst_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_u8_i32(value);
            }
            else
            {
                vx_int16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *dst = vx_clamp_s16_i32(value);
            }
        }
    }

    status |= vxUnmapImagePatch(src, map_id);
    status |= vxUnmapImagePatch(dst, result_map_id);

    return status;
}

// nodeless version of the Gaussian3x3 kernel
vx_status vxGaussian3x3(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_enum dst_format = VX_DF_IMAGE_VIRT;
    vx_status status = VX_SUCCESS;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y;
    uint16x8_t vCur[3], vNxt[3],vOrgCur[3];
    uint16x8_t vSum0, vSum1, vSum2, vSum3;
    vx_uint8 szData[8];
    uint16x8_t vTmp = vdupq_n_u16(0);

    status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id_src = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &map_id_dst, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

    high_y = src_addr.dim_y;
    high_x = src_addr.dim_x & 0xFFFFFFF8;
    if (bordermode->mode == VX_BORDER_UNDEFINED)
    {
        low_y = 1;
        high_y = src_addr.dim_y - 1;
    }

    for (y = low_y; y < high_y; y++)
    {
        for (vx_int8 idx = 0; idx < 3; idx++)
        {
            uint16x8_t vPrv = vdupq_n_u16(0);
            vx_int32 tmpY = y - 1 + idx;
            if (bordermode->mode == VX_BORDER_UNDEFINED || bordermode->mode == VX_BORDER_REPLICATE)
            {
                if (tmpY < 0)
                {
                    tmpY = 0;
                }
                if (tmpY >= src_addr.dim_y)
                {
                    tmpY = src_addr.dim_y - 1;
                }
                vPrv = vdupq_n_u16(*((vx_uint8 *)src_base + tmpY * src_addr.stride_y));
                vTmp = vmovl_u8(vld1_u8((vx_uint8 *)src_base + tmpY * src_addr.stride_y));
            }
            else if (bordermode->mode == VX_BORDER_CONSTANT)
            {
                vPrv = vdupq_n_u16(bordermode->constant_value.U8);
                if (tmpY < 0 || tmpY >= src_addr.dim_y)
                {
                    vTmp = vdupq_n_u16(bordermode->constant_value.U8);
                }
                else
                {
                    vTmp = vmovl_u8(vld1_u8((vx_uint8 *)src_base + tmpY * src_addr.stride_y));
                }
            }
            vCur[idx] = vextq_u16(vPrv, vTmp, 7);
            vOrgCur[idx] = vTmp;
        }
        vSum0 = vaddq_u16(vCur[0], vCur[2]);
        vSum1 = vshlq_n_u16(vCur[1], 1);

        for (x = low_x; x < high_x; x += 8)
        {
            if ((x + 8) < high_x)
            {
                for (vx_int8 idx = 0; idx < 3; idx++)
                {
                    vx_int32 tmpY = y - 1 + idx;
                    if (bordermode->mode == VX_BORDER_UNDEFINED || bordermode->mode == VX_BORDER_REPLICATE)
                    {
                        if (tmpY < 0)
                        {
                            tmpY = 0;
                        }
                        if (tmpY >= src_addr.dim_y)
                        {
                            tmpY = src_addr.dim_y - 1;
                        }

                        vTmp = vmovl_u8(vld1_u8((vx_uint8 *)src_base + tmpY * src_addr.stride_y + x + 8));
                    }
                    else if (bordermode->mode == VX_BORDER_CONSTANT)
                    {
                        if (tmpY < 0 || tmpY >= src_addr.dim_y)
                        {
                            vTmp = vdupq_n_u16(bordermode->constant_value.U8);
                        }
                        else
                        {
                            vTmp = vmovl_u8(vld1_u8((vx_uint8 *)src_base + tmpY * src_addr.stride_y + x + 8));
                        }
                    }
                    vNxt[idx] = vextq_u16(vOrgCur[idx], vTmp, 7);
                    vOrgCur[idx] = vTmp;
                }
            }
            else
            {
                for (vx_int8 idx = 0; idx < 3; idx++)
                {
                    vx_int32 tmpY = y - 1 + idx;
                    if (bordermode->mode == VX_BORDER_UNDEFINED || bordermode->mode == VX_BORDER_REPLICATE)
                    {
                        if (tmpY < 0)
                        {
                            tmpY = 0;
                        }
                        if (tmpY >= src_addr.dim_y)
                        {
                            tmpY = src_addr.dim_y - 1;
                        }
                        memcpy(szData, (vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x + 8),
                               src_addr.dim_x - (x + 8));
                        memset(&(szData[src_addr.dim_x - (x + 8)]),
                               *((vx_uint8 *)src_base + tmpY * src_addr.stride_y + src_addr.dim_x - 1),
                               8 - (src_addr.dim_x - (x + 8)));
                        vTmp = vmovl_u8(vld1_u8(szData));
                    }
                    else if (bordermode->mode == VX_BORDER_CONSTANT)
                    {
                        if (tmpY < 0 || tmpY >= src_addr.dim_y)
                        {
                            vTmp = vdupq_n_u16(bordermode->constant_value.U8);
                        }
                        else
                        {
                            memcpy(szData, (vx_uint8 *)src_base + tmpY * src_addr.stride_y + (x + 8),
                                   src_addr.dim_x - (x + 8));
                            memset(&(szData[src_addr.dim_x - (x + 8)]),
                                   bordermode->constant_value.U8,
                                   8 - (src_addr.dim_x - (x + 8)));
                            vTmp = vmovl_u8(vld1_u8(szData));
                        }
                    }
                    vNxt[idx] = vextq_u16(vOrgCur[idx], vTmp, 7);
                    vOrgCur[idx] = vTmp;
                }
            }
            vSum2 = vaddq_u16(vNxt[0], vNxt[2]);
            vSum3 = vshlq_n_u16(vNxt[1], 1);

            uint16x8_t vSum;
            vTmp = vextq_u16(vSum0, vSum2, 1);
            vSum = vmlaq_n_u16(vSum0, vTmp, 2);
            vSum = vaddq_u16(vSum, vSum1);
            vTmp = vextq_u16(vSum1, vSum3, 1);
            vSum = vmlaq_n_u16(vSum, vTmp, 2);
            vTmp = vextq_u16(vSum1, vSum3, 2);
            vSum = vaddq_u16(vSum, vTmp);
            vTmp = vextq_u16(vSum0, vSum2, 2);
            vSum = vaddq_u16(vSum, vTmp);
            vSum = vshrq_n_u16(vSum, 4);
            vst1_u8((vx_uint8 *)dst_base + y * dst_addr.stride_y + x, vmovn_u16(vSum));
            vSum0 = vSum2;
            vSum1 = vSum3;
        }

        if (x < high_x)
        {
            for (vx_int8 idx = 0; idx < 3; idx++)
            {
                vx_int32 tmpY = y - 1 + idx;
                if (bordermode->mode == VX_BORDER_UNDEFINED || bordermode->mode == VX_BORDER_REPLICATE)
                {
                    if (tmpY < 0)
                    {
                        tmpY = 0;
                    }
                    if (tmpY >= src_addr.dim_y)
                    {
                        tmpY = src_addr.dim_y - 1;
                    }
                    memset(&(szData[0]),
                           *((vx_uint8 *)src_base + tmpY * src_addr.stride_y + src_addr.dim_x - 1),
                           8);
                    vTmp = vmovl_u8(vld1_u8(szData));
                }
                else if (bordermode->mode == VX_BORDER_CONSTANT)
                {
                    vTmp = vdupq_n_u16(bordermode->constant_value.U8);
                }

                vNxt[idx] = vextq_u16(vOrgCur[idx], vTmp, 7);
            }

            vSum2 = vaddq_u16(vNxt[0], vNxt[2]);
            vSum3 = vshlq_n_u16(vNxt[1], 1);

            uint16x8_t vSum;
            vTmp = vextq_u16(vSum0, vSum2, 1);
            vSum = vmlaq_n_u16(vSum0, vTmp, 2);
            vSum = vaddq_u16(vSum, vSum1);
            vTmp = vextq_u16(vSum1, vSum3, 1);
            vSum = vmlaq_n_u16(vSum, vTmp, 2);
            vTmp = vextq_u16(vSum1, vSum3, 2);
            vSum = vaddq_u16(vSum, vTmp);
            vTmp = vextq_u16(vSum0, vSum2, 2);
            vSum = vaddq_u16(vSum, vTmp);
            vSum = vshrq_n_u16(vSum, 4);
            vst1_u8(szData, vmovn_u16(vSum));
            for (vx_uint8 idx = 0; idx < src_addr.dim_x - x; idx++)
            {
                *((vx_uint8 *)dst_base + y * dst_addr.stride_y + x + idx) = szData[idx];
            }
        }

    }
    status |= vxUnmapImagePatch(src, map_id_src);
    status |= vxUnmapImagePatch(dst, map_id_dst);
    return status;
}

