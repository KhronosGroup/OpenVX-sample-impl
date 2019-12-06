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

// nodeless version of the Threshold kernel
vx_status vxThreshold_U8(vx_image src_image, vx_threshold threshold, vx_image dst_image)
{
    vx_enum type = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t src_addr;
    vx_imagepatch_addressing_t dst_addr;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_uint32 y = 0;
    vx_uint32 x = 0;
    vx_pixel_value_t value;
    vx_pixel_value_t lower;
    vx_pixel_value_t upper;
    vx_pixel_value_t true_value;
    vx_pixel_value_t false_value;
    vx_status status = VX_FAILURE;
    vx_enum format = 0;

    vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));

    if (type == VX_THRESHOLD_TYPE_BINARY)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_VALUE, &value, sizeof(value));
        VX_PRINT(VX_ZONE_INFO, "threshold type binary, value = %u\n", value);
    }
    else if (type == VX_THRESHOLD_TYPE_RANGE)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_LOWER, &lower, sizeof(lower));
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_UPPER, &upper, sizeof(upper));
        VX_PRINT(VX_ZONE_INFO, "threshold type range, lower = %u, upper = %u\n", lower, upper);
    }

    vxQueryThreshold(threshold, VX_THRESHOLD_TRUE_VALUE, &true_value, sizeof(true_value));
    vxQueryThreshold(threshold, VX_THRESHOLD_FALSE_VALUE, &false_value, sizeof(false_value));
    VX_PRINT(VX_ZONE_INFO, "threshold true value = %u, threshold false value = %u\n", true_value, false_value);

    status = vxGetValidRegionImage(src_image, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(src_image, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst_image, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    vxQueryThreshold(threshold, VX_THRESHOLD_INPUT_FORMAT, &format, sizeof(format));
    VX_PRINT(VX_ZONE_INFO, "threshold input_format = %d\n", format);

    vx_uint8 true_value_u8 = true_value.U8;
    vx_uint8 false_value_u8 = false_value.U8;

    vx_uint8 _threshold_u8 = value.U8;
    vx_uint8 _lower_threshold_u8 = lower.U8;
    vx_uint8 _upper_threshold_u8 = upper.U8;

    vx_int16 _threshold_s16 = value.S16;
    vx_int16 _lower_threshold_s16 = lower.S16;
    vx_int16 _upper_threshold_s16 = upper.S16;

    if (format == VX_DF_IMAGE_S16)
    {//case of input: VX_DF_IMAGE_S16 -> output: VX_DF_IMAGE_U8
        vx_uint32 dim_x8 = src_addr.dim_x >> 3;

        const uint8x8_t true_value = vdup_n_u8(true_value_u8);
        const uint8x8_t false_value = vdup_n_u8(false_value_u8);

        const int16x8_t threshold = vdupq_n_s16(_threshold_s16);
        const int16x8_t lower_threshold = vdupq_n_s16(_lower_threshold_s16);
        const int16x8_t upper_threshold = vdupq_n_s16(_upper_threshold_s16);
        for (y = 0; y < src_addr.dim_y; y++)
        {
            const vx_int16 *src_ptr = (vx_int16 *)src_base + y * src_addr.stride_y / 2;
            vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * dst_addr.stride_y;

            if (type == VX_THRESHOLD_TYPE_BINARY)
            {
                for (x = 0; x < dim_x8; x++)
                {
                    const int16x8_t vSrc = vld1q_s16(src_ptr);
                    uint8x8_t mask = vmovn_u16(vcgtq_s16(vSrc, threshold));
                    uint8x8_t dst_value = vbsl_u8(mask, true_value, false_value);

                    vst1_u8(dst_ptr, dst_value);

                    src_ptr += 8;
                    dst_ptr += 8;
                }
                for (x = (dim_x8 << 3); x < src_addr.dim_x; x++)
                {
                    if (*src_ptr > _threshold_s16)
                    {
                        *dst_ptr = true_value_u8;
                    }
                    else
                    {
                        *dst_ptr = false_value_u8;
                    }

                    src_ptr++;
                    dst_ptr++;
                }
            }

            if (type == VX_THRESHOLD_TYPE_RANGE)
            {
                for (x = 0; x < dim_x8; x++)
                {
                    const int16x8_t vSrc = vld1q_s16(src_ptr);
                    uint16x8_t _mask = vcleq_s16(vSrc, upper_threshold);
                    _mask = vandq_u16(vcgeq_s16(vSrc, lower_threshold), _mask);
                    uint8x8_t mask = vmovn_u16(_mask);
                    vst1_u8(dst_ptr, vbsl_u8(mask, true_value, false_value));

                    src_ptr += 8;
                    dst_ptr += 8;
                }
                for (x = (dim_x8 << 3); x < src_addr.dim_x; x++)
                {
                    if (*src_ptr > _upper_threshold_s16)
                    {
                        *dst_ptr = false_value_u8;
                    }
                    else if (*src_ptr < _lower_threshold_s16)
                    {
                        *dst_ptr = false_value_u8;
                    }
                    else
                    {
                        *dst_ptr = true_value_u8;
                    }

                    src_ptr++;
                    dst_ptr++;
                }
            }
        }
    }
    else
    {//case of input: VX_DF_IMAGE_U8  -> output: VX_DF_IMAGE_U8
        vx_uint32 dim_x16 = src_addr.dim_x >> 4;

        const uint8x16_t true_value = vdupq_n_u8(true_value_u8);
        const uint8x16_t false_value = vdupq_n_u8(false_value_u8);

        const uint8x16_t threshold = vdupq_n_u8(_threshold_u8);
        const uint8x16_t lower_threshold = vdupq_n_u8(_lower_threshold_u8);
        const uint8x16_t upper_threshold = vdupq_n_u8(_upper_threshold_u8);

        for (y = 0; y < src_addr.dim_y; y++)
        {
            const vx_uint8 *src_ptr = (vx_uint8 *)src_base + y * src_addr.stride_y;
            vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * dst_addr.stride_y;

            if (type == VX_THRESHOLD_TYPE_BINARY)
            {
                for (x = 0; x < dim_x16; x++)
                {
                    const uint8x16_t vSrc = vld1q_u8(src_ptr);
                    uint8x16_t mask = vcgtq_u8(vSrc, threshold);
                    vst1q_u8(dst_ptr, vbslq_u8(mask, true_value, false_value));

                    src_ptr += 16;
                    dst_ptr += 16;
                }
                for (x = (dim_x16 << 4); x < src_addr.dim_x; x++)
                {
                    if (*src_ptr > _threshold_u8)
                    {
                        *dst_ptr = true_value_u8;
                    }
                    else
                    {
                        *dst_ptr = false_value_u8;
                    }

                    src_ptr++;
                    dst_ptr++;
                }
            }
            if (type == VX_THRESHOLD_TYPE_RANGE)
            {
                for (x = 0; x < dim_x16; x++)
                {
                    const uint8x16_t vSrc = vld1q_u8(src_ptr);
                    uint8x16_t mask = vcleq_u8(vSrc, upper_threshold);
                    mask = vandq_u8(vcgeq_u8(vSrc, lower_threshold), mask);
                    vst1q_u8(dst_ptr, vbslq_u8(mask, true_value, false_value));

                    src_ptr += 16;
                    dst_ptr += 16;
                }
                for (x = (dim_x16 << 4); x < src_addr.dim_x; x++)
                {
                    if (*src_ptr > _upper_threshold_u8)
                    {
                        *dst_ptr = false_value_u8;
                    }
                    else if (*src_ptr < _lower_threshold_u8)
                    {
                        *dst_ptr = false_value_u8;
                    }
                    else
                    {
                        *dst_ptr = true_value_u8;
                    }

                    src_ptr++;
                    dst_ptr++;
                }
            }
        }
    }

    status |= vxUnmapImagePatch(src_image, map_id);
    status |= vxUnmapImagePatch(dst_image, result_map_id);

    return status;
}

vx_status vxThreshold_U1(vx_image src_image, vx_threshold threshold, vx_image dst_image)
{
    vx_enum type = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t src_addr;
    vx_imagepatch_addressing_t dst_addr;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_uint32 y = 0;
    vx_uint32 x = 0;
    vx_pixel_value_t value;
    vx_pixel_value_t lower;
    vx_pixel_value_t upper;
    vx_pixel_value_t true_value;
    vx_pixel_value_t false_value;
    vx_status status = VX_FAILURE;
    vx_enum in_format  = 0;
    vx_enum out_format = 0;

    vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));

    if (type == VX_THRESHOLD_TYPE_BINARY)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_VALUE, &value, sizeof(value));
        VX_PRINT(VX_ZONE_INFO, "threshold type binary, value = %u\n", value);
    }
    else if (type == VX_THRESHOLD_TYPE_RANGE)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_LOWER, &lower, sizeof(lower));
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_UPPER, &upper, sizeof(upper));
        VX_PRINT(VX_ZONE_INFO, "threshold type range, lower = %u, upper = %u\n", lower, upper);
    }

    vxQueryThreshold(threshold, VX_THRESHOLD_TRUE_VALUE, &true_value, sizeof(true_value));
    vxQueryThreshold(threshold, VX_THRESHOLD_FALSE_VALUE, &false_value, sizeof(false_value));
    VX_PRINT(VX_ZONE_INFO, "threshold true value = %u, threshold false value = %u\n", true_value, false_value);

    status = vxGetValidRegionImage(src_image, &rect);
    status |= vxAccessImagePatch(src_image, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    vxQueryThreshold(threshold, VX_THRESHOLD_INPUT_FORMAT,  &in_format,  sizeof(in_format));
    vxQueryThreshold(threshold, VX_THRESHOLD_OUTPUT_FORMAT, &out_format, sizeof(out_format));
    VX_PRINT(VX_ZONE_INFO, "threshold: input_format  = %d, output_format = %d\n", in_format, out_format);

    for (y = 0; y < src_addr.dim_y; y++)
    {
        for (x = 0; x < src_addr.dim_x; x++)
        {
            vx_uint32 xShftd = x + (out_format == VX_DF_IMAGE_U1 ? rect.start_x % 8 : 0);
            void *src_void_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            void *dst_void_ptr = vxFormatImagePatchAddress2d(dst_base, xShftd, y, &dst_addr);

            if( in_format == VX_DF_IMAGE_S16 )
            {//case of input: VX_DF_IMAGE_S16

                vx_int16 *src_ptr = (vx_int16*)src_void_ptr;
                vx_uint8 *dst_ptr = (vx_uint8*)dst_void_ptr;

                vx_uint8 dst_value = 0;

                if (type == VX_THRESHOLD_TYPE_BINARY)
                {
                    if (*src_ptr > value.S16)
                        dst_value = true_value.U1  ? 1 : 0;
                    else
                        dst_value = false_value.U1 ? 1 : 0;

                    vx_uint8 offset = xShftd % 8;
                    *dst_ptr = (*dst_ptr & ~(1 << offset)) | (dst_value << offset);
                }

                if (type == VX_THRESHOLD_TYPE_RANGE)
                {
                    if (*src_ptr > upper.S16)
                        dst_value = false_value.U1 ? 1 : 0;
                    else if (*src_ptr < lower.S16)
                        dst_value = false_value.U1 ? 1 : 0;
                    else
                        dst_value = true_value.U1  ? 1 : 0;

                    vx_uint8 offset = xShftd % 8;
                    *dst_ptr = (*dst_ptr & ~(1 << offset)) | (dst_value << offset);
                }
            }
            else
            {//case of input: VX_DF_IMAGE_U8

                vx_uint8 *src_ptr = (vx_uint8*)src_void_ptr;
                vx_uint8 *dst_ptr = (vx_uint8*)dst_void_ptr;

                if (type == VX_THRESHOLD_TYPE_BINARY)
                {
                    vx_uint8 offset = xShftd % 8;
                    if (*src_ptr > value.U8)
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((true_value.U1  ? 1 : 0) << offset);
                    else
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value.U8 ? 1 : 0) << offset);
                }

                if (type == VX_THRESHOLD_TYPE_RANGE)
                {
                    vx_uint8 offset = xShftd % 8;
                    if (*src_ptr > upper.U8)
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value.U1 ? 1 : 0) << offset);
                    else if (*src_ptr < lower.U8)
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value.U1 ? 1 : 0) << offset);
                    else
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((true_value.U1  ? 1 : 0) << offset);
                }
            } //end if else
        }//end for
    }//end for

    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &rect, 0, &dst_addr, dst_base);

    return status;
}


vx_status vxThreshold(vx_image src_image, vx_threshold threshold, vx_image dst_image)
{
    vx_df_image out_format;
    vx_status status = vxQueryThreshold(threshold, VX_THRESHOLD_OUTPUT_FORMAT, &out_format, sizeof(out_format));
    if (status != VX_SUCCESS)
        return status;
    else if (out_format == VX_DF_IMAGE_U1)
        return vxThreshold_U1(src_image, threshold, dst_image);
    else
        return vxThreshold_U8(src_image, threshold, dst_image);
}
