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

static vx_int16 sobel_x[3][3] = {
    { -1, 0, +1 },
    { -2, 0, +2 },
    { -1, 0, +1 },
};

static vx_int16 sobel_y[3][3] = {
    { -1, -2, -1 },
    { 0,  0,  0 },
    { +1, +2, +1 },
};

static vx_status vxConvolution3x3(vx_image src, vx_image dst, const vx_border_t *bordermode,vx_uint8 vx_x,vx_uint8 vx_y)
{
    vx_uint32 y,x;
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
    status |= vxMapImagePatch(dst, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

    high_x = src_addr.dim_x;
    high_y = src_addr.dim_y;

    vx_int32 src_stride_y = src_addr.stride_y;
    vx_int32 dst_stride_y = dst_addr.stride_y;

    int16x8_t two      = vdupq_n_s16(2);
    int16x8_t minustwo = vdupq_n_s16(-2);

    ++low_x; --high_x;
    ++low_y; --high_y;
    vxAlterRectangle(&rect, 1, 1, -1, -1);

    vx_uint32 width_x8 = high_x / 8 * 8;

    for (y = low_y; y < high_y; y++)
    {
        vx_int16* dstp = (vx_int16 *)dst_base + low_x + y * dst_addr.dim_x;
        vx_uint8* top_src = (vx_uint8 *)src_base + (y - 1) * src_addr.dim_x;
        vx_uint8* mid_src = (vx_uint8 *)src_base + (y)* src_addr.dim_x;
        vx_uint8* bot_src = (vx_uint8 *)src_base + (y + 1)* src_addr.dim_x;

        for (x = low_x; x < width_x8; x += 8,top_src += 8, mid_src += 8, bot_src += 8,dstp += 8)
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
            
            if (vx_y)
            {
                //SOBEL Y
                //top left
                int16x8_t out_y = vnegq_s16(top_s16.val[0]);
                //top mid
                out_y = vmlaq_s16(out_y, vextq_s16(top_s16.val[0], top_s16.val[1], 1), minustwo);
                //top right
                out_y = vsubq_s16(out_y, vextq_s16(top_s16.val[0], top_s16.val[1], 2));
                //bot left
                out_y = vaddq_s16(out_y, bot_s16.val[0]);
                //bot mid
                out_y = vmlaq_s16(out_y, vextq_s16(bot_s16.val[0], bot_s16.val[1], 1), two);
                //bot right
                out_y = vaddq_s16(out_y, vextq_s16(bot_s16.val[0], bot_s16.val[1], 2));

                vst1q_s16(dstp, out_y);
            }
            if (vx_x)
            {
                //SOBEL X
                //top left
                int16x8_t out_x = vnegq_s16(top_s16.val[0]);
                //top right
                out_x = vaddq_s16(out_x, vextq_s16(top_s16.val[0], top_s16.val[1], 2));
                //mid left
                out_x = vmlaq_s16(out_x, mid_s16.val[0], minustwo);
                //mid right
                out_x = vmlaq_s16(out_x, vextq_s16(mid_s16.val[0], mid_s16.val[1], 2), two);
                //bot left
                out_x = vsubq_s16(out_x, bot_s16.val[0]);
                //bot right
                out_x = vaddq_s16(out_x, vextq_s16(bot_s16.val[0], bot_s16.val[1], 2));

                vst1q_s16(dstp, out_x);
            }
        }

        for (; x < high_x; x++)
        {
            if (vx_y)
            {
                vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_y, bordermode);

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
            if (vx_x)
            {
                vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_x, bordermode);

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
    }

    if (bordermode->mode == VX_BORDER_REPLICATE || bordermode->mode == VX_BORDER_CONSTANT)
    {
        ++high_x;
        ++high_y;

        if (vx_y)
        {
            //Calculate border
            for (y = 0; y < high_y; y++)
            {
                x = 0;
                vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_y, bordermode);

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
                value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_y, bordermode);

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
                vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_y, bordermode);

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
                value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_y, bordermode);

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
        if (vx_x)
        {
            //Calculate border
            for (y = 0; y < high_y; y++)
            {
                x = 0;
                vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_x, bordermode);

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
                value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_x, bordermode);

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
                vx_int32 value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_x, bordermode);

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
                value = vx_convolve8with16(src_base, x, y, &src_addr, sobel_x, bordermode);

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
    }

    status |= vxUnmapImagePatch(src, map_id);
    status |= vxUnmapImagePatch(dst, result_map_id);

    return status;

}

// nodeless version of the Sobel3x3 kernelHOGFeatures
vx_status vxSobel3x3(vx_image input, vx_image grad_x, vx_image grad_y, vx_border_t *bordermode)
{
    if (grad_x) {
        vx_status status = vxConvolution3x3(input, grad_x, bordermode,1,0);
        if (status != VX_SUCCESS) return status;
    }

    if (grad_y) {
        vx_status status = vxConvolution3x3(input, grad_y, bordermode,0,1);
        if (status != VX_SUCCESS) return status;
    }

    return VX_SUCCESS;   

}
