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
#include <stdlib.h>

#include <stdio.h>

static vx_status Gaussian5x5_U8_U8(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    vx_int32 y, x, i;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_size conv_width = 5, conv_height = 5;
    vx_int32 conv_radius_x, conv_radius_y;
    vx_int16 conv_mat[25] = { 1, 4, 6, 4, 1, 4, 16, 24, 16, 4, 6, 24, 36, 24, 6, 4, 16, 24, 16, 4, 1, 4, 6, 4, 1 };
    vx_int32 sum = 0, value = 0;

    vx_df_image src_format = 0;
    vx_df_image dst_format = 0;
    vx_status status = VX_SUCCESS;
    vx_int32 low_x, low_y, high_x, high_y;

    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    conv_radius_x = (vx_int32)conv_width / 2;
    conv_radius_y = (vx_int32)conv_height / 2;
    status |= vxGetValidRegionImage(src, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    vx_uint32 srcStride = src_addr.stride_y;
    vx_uint32 dstStride = dst_addr.stride_y;

    low_x = 0;
    high_x = src_addr.dim_x;
    low_y = 0;
    high_y = src_addr.dim_y;

    int16x8_t vsix = vdupq_n_s16(6);
    int16x8_t vfour = vdupq_n_s16(4);

    vx_uint32 width = src_addr.dim_x;
    vx_uint32 height = src_addr.dim_y;

    vx_int16 * mid_dst = (vx_int16 *)malloc(width * height * sizeof(vx_int16));

    vx_uint32 width_x8 = width / 8 * 8;

    for (y = low_y; y < high_y; ++y)
    {
        vx_uint8* srcp = (vx_uint8 *)src_base + y * srcStride;

        vx_int16* mid_dstp = (vx_int16 *)mid_dst + y * dstStride;

        x = low_x;
        for (; x < width_x8; x += 8)
        {
            uint8x16_t data = vld1q_u8(srcp);

            const int16x8x2_t data_s16 =
            {
                {
                    vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(data))),
                    vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(data)))
                }
            };

            int16x8_t out = vaddq_s16(data_s16.val[0], vextq_s16(data_s16.val[0], data_s16.val[1], 4));
            out = vmlaq_s16(out, vextq_s16(data_s16.val[0], data_s16.val[1], 1), vfour);
            out = vmlaq_s16(out, vextq_s16(data_s16.val[0], data_s16.val[1], 2), vsix);
            out = vmlaq_s16(out, vextq_s16(data_s16.val[0], data_s16.val[1], 3), vfour);

            vst1q_s16(mid_dstp, out);

            srcp += 8;
            mid_dstp += 8;
        }
        for (; x < width; x++)
        {
            sum = 0;

            sum += srcp[0];//1
            sum += srcp[1] * 4;//4
            sum += srcp[2] * 6;//6
            sum += srcp[3] * 4;//4
            sum += srcp[4];//1

            if (sum < INT16_MIN) *mid_dstp = INT16_MIN;
            else if (sum > INT16_MAX) *mid_dstp = INT16_MAX;
            else *mid_dstp = sum;
            srcp++;
            mid_dstp++;
        }
    }

    low_x = conv_radius_x;
    high_x = ((src_addr.dim_x >= (vx_uint32)conv_radius_x) ? src_addr.dim_x - conv_radius_x : 0);
    low_y = conv_radius_y;
    high_y = ((src_addr.dim_y >= (vx_uint32)conv_radius_y) ? src_addr.dim_y - conv_radius_y : 0);

    uint16x8_t six = vdupq_n_u16(6);
    uint16x8_t four = vdupq_n_u16(4);

    const size_t input_offset_low_s16 = 8;

    vx_uint32 width_x16 = high_x / 16 * 16;

    for (y = low_y; y < high_y; ++y)
    {
        vx_int16* top_src2 = (vx_int16 *)mid_dst + (y - 2) * srcStride;
        vx_int16* top_src1 = (vx_int16 *)mid_dst + (y - 1) * srcStride;
        vx_int16* mid_src = (vx_int16 *)mid_dst + (y)* srcStride;
        vx_int16* bot_src1 = (vx_int16 *)mid_dst + (y + 1) * srcStride;
        vx_int16* bot_src2 = (vx_int16 *)mid_dst + (y + 2) * srcStride;

        vx_uint8* dstp = (vx_uint8 *)dst_base + low_x + y * dstStride;
        x = low_x;
        for (; x < width_x16; x += 16)
        {
            //HIGH DATA
            //top2
            uint16x8_t data_high = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(top_src2)));
            uint16x8_t out_high = data_high;
            //top
            data_high = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(top_src1)));
            out_high = vmlaq_u16(out_high, data_high, four);
            //mid
            data_high = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(mid_src)));
            out_high = vmlaq_u16(out_high, data_high, six);
            //low
            data_high = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(bot_src1)));
            out_high = vmlaq_u16(out_high, data_high, four);
            //low2
            data_high = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(bot_src2)));
            out_high = vaddq_u16(out_high, data_high);

            //LOW DATA
            //top2
            uint16x8_t data_low = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(top_src2 + input_offset_low_s16)));
            uint16x8_t out_low = data_low;
            //top
            data_low = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(top_src1 + input_offset_low_s16)));
            out_low = vmlaq_u16(out_low, data_low, four);
            //mid
            data_low = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(mid_src + input_offset_low_s16)));
            out_low = vmlaq_u16(out_low, data_low, six);
            //low
            data_low = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(bot_src1 + input_offset_low_s16)));
            out_low = vmlaq_u16(out_low, data_low, four);
            //low2
            data_low = vreinterpretq_u16_s16(vld1q_s16((vx_int16 *)(bot_src2 + input_offset_low_s16)));
            out_low = vaddq_u16(out_low, data_low);

            vst1q_u8(dstp, vcombine_u8(vqshrn_n_u16(out_high, 8), vqshrn_n_u16(out_low, 8)));

            top_src2 += 16;
            top_src1 += 16;
            mid_src += 16;
            bot_src1 += 16;
            bot_src2 += 16;
            dstp += 16;
        }
        for (; x < high_x; x++)
        {
            sum = 0;

            sum += top_src2[0];//1
            sum += top_src1[0] * 4;//4
            sum += mid_src[0] * 6;//6
            sum += bot_src1[0] * 4;//4
            sum += bot_src2[0];//1

            value = sum >> 8;

            if (value < 0) *dstp = 0;
            else if (value > UINT8_MAX) *dstp = UINT8_MAX;
            else *dstp = value;

            top_src2++;
            top_src1++;
            mid_src++;
            bot_src1++;
            bot_src2++;
            dstp++;
        }
    }

    free(mid_dst);

    // Calculate border for REPLICATE and CONSTANT border mode
    if (bordermode->mode == VX_BORDER_REPLICATE || bordermode->mode == VX_BORDER_CONSTANT)
    {
        // Calculate the left and right border
        for (y = 0; y < high_y + conv_radius_y; ++y)
        {
            for (x = 0; x < conv_radius_x; ++x)
            {
                sum = 0;

                vx_uint8 slice[25] = { 0 };

                vxReadRectangle(src_base, &src_addr, bordermode, src_format, x, y, conv_radius_x, conv_radius_y, slice, 0);

                for (i = 0; i < (vx_int32)(conv_width * conv_height); ++i)
                    sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];

                value = sum / 256;

                vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                if (value < 0) *dstp = 0;
                else if (value > UINT8_MAX) *dstp = UINT8_MAX;
                else *dstp = value;
            }

            for (x = high_x; x < high_x + conv_radius_x; ++x)
            {
                sum = 0;

                vx_uint8 slice[25] = { 0 };

                vxReadRectangle(src_base, &src_addr, bordermode, src_format, x, y, conv_radius_x, conv_radius_y, slice, 0);

                for (i = 0; i < (vx_int32)(conv_width * conv_height); ++i)
                    sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];

                value = sum / 256;

                vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                if (value < 0) *dstp = 0;
                else if (value > UINT8_MAX) *dstp = UINT8_MAX;
                else *dstp = value;
            }
        }

        for (x = conv_radius_x; x < high_x; ++x)
        {
            for (y = 0; y < conv_radius_y; ++y)
            {
                sum = 0;

                vx_uint8 slice[25] = { 0 };

                vxReadRectangle(src_base, &src_addr, bordermode, src_format, x, y, conv_radius_x, conv_radius_y, slice, 0);

                for (i = 0; i < (vx_int32)(conv_width * conv_height); ++i)
                    sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];

                value = sum / 256;

                vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                if (value < 0) *dstp = 0;
                else if (value > UINT8_MAX) *dstp = UINT8_MAX;
                else *dstp = value;
            }

            for (y = high_y; y < high_y + conv_radius_y; ++y)
            {
                sum = 0;

                vx_uint8 slice[25] = { 0 };

                vxReadRectangle(src_base, &src_addr, bordermode, src_format, x, y, conv_radius_x, conv_radius_y, slice, 0);

                for (i = 0; i < (vx_int32)(conv_width * conv_height); ++i)
                    sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];

                value = sum / 256;

                vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                if (value < 0) *dstp = 0;
                else if (value > UINT8_MAX) *dstp = UINT8_MAX;
                else *dstp = value;
            }
        }
    }

    status |= vxUnmapImagePatch(src, map_id);
    status |= vxUnmapImagePatch(dst, result_map_id);

    return status;
}

#define C_MAX_CONVOLUTION_DIM (15)
#define CONV_DIM 5
#define CONV_DIM_HALF CONV_DIM / 2

#define INSERT_ZERO_Y(slice, y) for (int i=0; i<CONV_DIM; i++) *(slice+CONV_DIM*(1-y)+i) = 0;
#define INSERT_VALUES_Y(slice, y) for (int i=0; i<CONV_DIM; i++) *(slice+CONV_DIM*(high_y-y)+i+CONV_DIM_HALF*CONV_DIM) = *(slice+CONV_DIM*(high_y-y)+i);
#define INSERT_ZERO_X(slice, x) for (int i=0; i<CONV_DIM; i++) *(slice+CONV_DIM*i+1-x) = 0;
#define INSERT_VALUES_X(slice, x) for (int i=0; i<CONV_DIM; i++) *(slice+CONV_DIM*i+(high_x-x)+CONV_DIM_HALF) = *(slice+CONV_DIM*i+(high_x-x));

static vx_status Gaussian5x5_U8_S16(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    vx_int32 y, x, i;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_size conv_width = 5, conv_height = 5;
    vx_int32 conv_radius_x, conv_radius_y;
    vx_int16 conv_mat[25] = { 1, 4, 6, 4, 1, 4, 16, 24, 16, 4, 6, 24, 36, 24, 6, 4, 16, 24, 16, 4, 1, 4, 6, 4, 1 };
    vx_int32 sum = 0, value = 0;
    vx_uint32 scale = 1;
    vx_df_image src_format = 0;
    vx_df_image dst_format = 0;
    vx_status status = VX_SUCCESS;
    vx_int32 low_x, low_y, high_x, high_y;

    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    conv_radius_x = (vx_int32)conv_width / 2;
    conv_radius_y = (vx_int32)conv_height / 2;
    status |= vxGetValidRegionImage(src, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    if (bordermode->mode == VX_BORDER_UNDEFINED)
    {
        low_x = conv_radius_x;
        high_x = ((src_addr.dim_x >= (vx_uint32)conv_radius_x) ? src_addr.dim_x - conv_radius_x : 0);
        low_y = conv_radius_y;
        high_y = ((src_addr.dim_y >= (vx_uint32)conv_radius_y) ? src_addr.dim_y - conv_radius_y : 0);
        vxAlterRectangle(&rect, conv_radius_x, conv_radius_y, -conv_radius_x, -conv_radius_y);
    }
    else
    {
        low_x = 0;
        high_x = src_addr.dim_x;
        low_y = 0;
        high_y = src_addr.dim_y;
    }
    for (y = low_y; y < high_y; ++y)
    {
        for (x = low_x; x < high_x; ++x)
        {
            sum = 0;

            if (src_format == VX_DF_IMAGE_U8)
            {
                vx_uint8 slice[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = {0};

                vxReadRectangle(src_base, &src_addr, bordermode, src_format, x, y, conv_radius_x, conv_radius_y, slice, 0);
                if (y < CONV_DIM_HALF)
                {
                    INSERT_ZERO_Y(slice, y)
                }
                else if (y >= high_y - CONV_DIM_HALF)
                {
                    INSERT_VALUES_Y(slice, y)
                }

                if (x < CONV_DIM_HALF)
                {
                    INSERT_ZERO_X(slice, x)
                }
                else if (x >= high_x - CONV_DIM_HALF)
                {
                    INSERT_VALUES_X(slice, x)
                }
                for (i = 0; i < (vx_int32)(conv_width * conv_height); ++i)
                    sum += conv_mat[conv_width * conv_height - 1 - i] * slice[i];
            }

            value = sum / 256;
            vx_int16 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            if (value < INT16_MIN) *dstp = INT16_MIN;
            else if (value > INT16_MAX) *dstp = INT16_MAX;
            else *dstp = value;
        }
    }
    status |= vxUnmapImagePatch(src, map_id);
    status |= vxUnmapImagePatch(dst, result_map_id);

    return status;
}

vx_status vxGaussian5x5(vx_image src, vx_image dst, vx_border_t *bordermode)
{
    vx_df_image src_format = 0;
    vx_df_image dst_format = 0;
    vx_status status = VX_SUCCESS;

    vx_uint32 width = 0;
    vx_uint32 height = 0;

    vxQueryImage(src, VX_IMAGE_WIDTH, &width, sizeof(width));
    vxQueryImage(src, VX_IMAGE_HEIGHT, &height, sizeof(height));

    status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

    if (src_format == VX_DF_IMAGE_U8 && dst_format == VX_DF_IMAGE_U8)
    {
        Gaussian5x5_U8_U8(src, dst, bordermode);
    }
    else if (src_format == VX_DF_IMAGE_U8 && dst_format == VX_DF_IMAGE_S16)
    {
        Gaussian5x5_U8_S16(src, dst, bordermode);
    }
    return status;
}
