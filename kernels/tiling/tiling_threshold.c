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

void Threshold_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_threshold_t *threshold = (vx_tile_threshold_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint8 true_value_u8 = threshold->true_value.U8;
    vx_uint8 false_value_u8 = threshold->false_value.U8;

    vx_uint8 _threshold_u8 = threshold->value.U8;
    vx_uint8 _lower_threshold_u8 = threshold->lower.U8;
    vx_uint8 _upper_threshold_u8 = threshold->upper.U8;

    vx_int16 _threshold_s16 = threshold->value.S16;
    vx_int16 _lower_threshold_s16 = threshold->lower.S16;
    vx_int16 _upper_threshold_s16 = threshold->upper.S16;

    vx_int32 format = threshold->input_format;
    vx_int32 type = threshold->thresh_type;

    if (format == VX_DF_IMAGE_S16)
    {//case of input: VX_DF_IMAGE_S16 -> output: VX_DF_IMAGE_U8
        vx_int16 *src_base = (vx_int16 *)in->base[0] + in->tile_x;
        vx_uint8 *dst_base = out->base[0] + out->tile_x;

        const uint8x8_t true_value = vdup_n_u8(true_value_u8);
        const uint8x8_t false_value = vdup_n_u8(false_value_u8);

        const int16x8_t threshold = vdupq_n_s16(_threshold_s16);
        const int16x8_t lower_threshold = vdupq_n_s16(_lower_threshold_s16);
        const int16x8_t upper_threshold = vdupq_n_s16(_upper_threshold_s16);

        if (type == VX_THRESHOLD_TYPE_BINARY)
        {
            for (y = low_y; y < high_y; y++)
            {
                const vx_int16 *src_ptr = (vx_int16 *)src_base + y * in->addr->stride_y / 2;
                vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * out->addr->stride_y;  

                for (x = 0; x < out->tile_block.width; x += 8)
                {
                    const int16x8_t vSrc = vld1q_s16(src_ptr);
                    uint8x8_t mask = vmovn_u16(vcgtq_s16(vSrc, threshold));
                    uint8x8_t dst_value = vbsl_u8(mask, true_value, false_value);

                    vst1_u8(dst_ptr, dst_value);

                    src_ptr += 8;
                    dst_ptr += 8;
                }
            }
        }
        else if (type == VX_THRESHOLD_TYPE_RANGE)
        {
            for (y = low_y; y < high_y; y++)
            {
                const vx_int16 *src_ptr = (vx_int16 *)src_base + y * in->addr->stride_y / 2;
                vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * out->addr->stride_y;
                for (x = 0; x < out->tile_block.width; x += 8)
                {
                    const int16x8_t vSrc = vld1q_s16(src_ptr);
                    uint16x8_t _mask = vcleq_s16(vSrc, upper_threshold);
                    _mask = vandq_u16(vcgeq_s16(vSrc, lower_threshold), _mask);
                    uint8x8_t mask = vmovn_u16(_mask);
                    vst1_u8(dst_ptr, vbsl_u8(mask, true_value, false_value));

                    src_ptr += 8;
                    dst_ptr += 8;
                }
            }
        }
    }
    else
    {//case of input: VX_DF_IMAGE_U8  -> output: VX_DF_IMAGE_U8
        vx_uint8 *src_base = in->base[0] + in->tile_x;
        vx_uint8 *dst_base = out->base[0] + out->tile_x;

        const uint8x16_t true_value = vdupq_n_u8(true_value_u8);
        const uint8x16_t false_value = vdupq_n_u8(false_value_u8);

        const uint8x16_t threshold = vdupq_n_u8(_threshold_u8);
        const uint8x16_t lower_threshold = vdupq_n_u8(_lower_threshold_u8);
        const uint8x16_t upper_threshold = vdupq_n_u8(_upper_threshold_u8);

        if (type == VX_THRESHOLD_TYPE_BINARY)
        {
            for (y = low_y; y < high_y; y++)
            {
                const vx_uint8 *src_ptr = (vx_uint8 *)src_base + y * in->addr->stride_y;
                vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * out->addr->stride_y;

                for (x = 0; x < out->tile_block.width; x += 16)
                {
                    const uint8x16_t vSrc = vld1q_u8(src_ptr);
                    uint8x16_t mask = vcgtq_u8(vSrc, threshold);
                    vst1q_u8(dst_ptr, vbslq_u8(mask, true_value, false_value));

                    src_ptr += 16;
                    dst_ptr += 16;
                }
            }
        }
        else if (type == VX_THRESHOLD_TYPE_RANGE)
        {
            for (y = low_y; y < high_y; y++)
            {
                const vx_uint8 *src_ptr = (vx_uint8 *)src_base + y * in->addr->stride_y;
                vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * out->addr->stride_y;

                for (x = 0; x < out->tile_block.width; x += 16)
                {
                    const uint8x16_t vSrc = vld1q_u8(src_ptr);
                    uint8x16_t mask = vcleq_u8(vSrc, upper_threshold);
                    mask = vandq_u8(vcgeq_u8(vSrc, lower_threshold), mask);
                    vst1q_u8(dst_ptr, vbslq_u8(mask, true_value, false_value));

                    src_ptr += 16;
                    dst_ptr += 16;
                }
            }
        }
    }
}



#define vxThreshold_BINARY(type, low_y, high_y, low_x, high_x, type_size)                       \
    for (y = low_y; y < high_y; y++)                                                            \
    {                                                                                           \
        const type *src_ptr = (type *)src_base + y * in->addr->stride_y / type_size;            \
        vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * out->addr->stride_y;                     \
                                                                                                \
        for (x = low_x; x < high_x; x++)                                                        \
        {                                                                                       \
            if (*src_ptr > _threshold_s16)                                                      \
            {                                                                                   \
                *dst_ptr = true_value_u8;                                                       \
            }                                                                                   \
            else                                                                                \
            {                                                                                   \
                *dst_ptr = false_value_u8;                                                      \
            }                                                                                   \
            src_ptr++;                                                                          \
            dst_ptr++;                                                                          \
        }                                                                                       \
    }                                                                                           \


#define vxThreshold_RANGE(type, low_y, high_y, low_x, high_x, type_size)                        \
    for (y = low_y; y < high_y; y++)                                                            \
    {                                                                                           \
        const type *src_ptr = (type *)src_base + y * in->addr->stride_y / type_size;            \
        vx_uint8 *dst_ptr = (vx_uint8 *)dst_base + y * out->addr->stride_y;                     \
                                                                                                \
        for (x = low_x; x < high_x; x++)                                                        \
        {                                                                                       \
            if (*src_ptr > _upper_threshold_s16)                                                \
            {                                                                                   \
                *dst_ptr = false_value_u8;                                                      \
            }                                                                                   \
            else if (*src_ptr < _lower_threshold_s16)                                           \
            {                                                                                   \
                *dst_ptr = false_value_u8;                                                      \
            }                                                                                   \
            else                                                                                \
            {                                                                                   \
                *dst_ptr = true_value_u8;                                                       \
            }                                                                                   \
            src_ptr++;                                                                          \
            dst_ptr++;                                                                          \
        }                                                                                       \
    }                                                                                           \


void Threshold_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_threshold_t *threshold = (vx_tile_threshold_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    vx_uint8 true_value_u8 = threshold->true_value.U8;
    vx_uint8 false_value_u8 = threshold->false_value.U8;

    vx_bool true_value_u1 = threshold->true_value.U1;
    vx_bool false_value_u1 = threshold->false_value.U1;

    vx_uint8 _threshold_u8 = threshold->value.U8;
    vx_uint8 _lower_threshold_u8 = threshold->lower.U8;
    vx_uint8 _upper_threshold_u8 = threshold->upper.U8;

    vx_int16 _threshold_s16 = threshold->value.S16;
    vx_int16 _lower_threshold_s16 = threshold->lower.S16;
    vx_int16 _upper_threshold_s16 = threshold->upper.S16;

    vx_int32 format = threshold->input_format;
    vx_int32 out_format = threshold->output_format;
    vx_int32 type = threshold->thresh_type;

    if (out_format != VX_DF_IMAGE_U1)
    {
        if (format == VX_DF_IMAGE_S16)
        {//case of input: VX_DF_IMAGE_S16 -> output: VX_DF_IMAGE_U8
            vx_int16 *src_base = (vx_int16 *)in->base[0] + in->tile_x;
            vx_uint8 *dst_base = out->base[0] + out->tile_x;
            if (type == VX_THRESHOLD_TYPE_BINARY)
            {
                if (low_y == 0 && low_x == 0)
                {
                    vxThreshold_BINARY(vx_int16, low_y, high_y, low_x, high_x, 2)
                }
                else
                {
                    vxThreshold_BINARY(vx_int16, 0, low_y, low_x, high_x, 2)

                    src_base = (vx_int16 *)in->base[0];
                    dst_base = out->base[0];
                    vxThreshold_BINARY(vx_int16, low_y, high_y, 0, high_x, 2)
                }
            }
            else if (type == VX_THRESHOLD_TYPE_RANGE)
            {
                if (low_y == 0 && low_x == 0)
                {
                    vxThreshold_RANGE(vx_int16, low_y, high_y, low_x, high_x, 2)
                }
                else
                {
                    vxThreshold_RANGE(vx_int16, 0, low_y, low_x, high_x, 2)

                    src_base = (vx_int16 *)in->base[0];
                    dst_base = out->base[0];
                    vxThreshold_RANGE(vx_int16, low_y, high_y, 0, high_x, 2)
                }
            }
        }
        else
        {//case of input: VX_DF_IMAGE_U8  -> output: VX_DF_IMAGE_U8
            vx_uint8 *src_base = in->base[0] + in->tile_x;
            vx_uint8 *dst_base = out->base[0] + out->tile_x;
            if (type == VX_THRESHOLD_TYPE_BINARY)
            {
                if (low_y == 0 && low_x == 0)
                {
                    vxThreshold_BINARY(vx_uint8, low_y, high_y, low_x, high_x, 1)
                }
                else
                {
                    vxThreshold_BINARY(vx_uint8, 0, low_y, low_x, high_x, 1)

                    src_base = in->base[0];
                    dst_base = out->base[0];
                    vxThreshold_BINARY(vx_uint8, low_y, high_y, 0, high_x, 1)
                }
            }
            else if (type == VX_THRESHOLD_TYPE_RANGE)
            {
                if (low_y == 0 && low_x == 0)
                {
                    vxThreshold_RANGE(vx_uint8, low_y, high_y, low_x, high_x, 1)
                }
                else
                {
                    vxThreshold_RANGE(vx_uint8, 0, low_y, low_x, high_x, 1)

                    src_base = in->base[0];
                    dst_base = out->base[0];
                    vxThreshold_RANGE(vx_uint8, low_y, high_y, 0, high_x, 1)
                }
            }
        }
    }
    else
    {
        void *src_base = in->base[0];                                                           
        void *dst_base = out->base[0];
        high_y = in->rect.end_y - in->rect.start_y;
        high_x = in->rect.end_x - in->rect.start_x;
        for (y = 0; y < high_y; y++)
        {
            for (x = 0; x < high_x; x++)
            {
                vx_uint32 xShftd = x + in->rect.start_x % 8;
                void *src_void_ptr = vxFormatImagePatchAddress2d(src_base, x, y, in->addr);
                void *dst_void_ptr = vxFormatImagePatchAddress2d(dst_base, xShftd, y, out->addr);

                if (format == VX_DF_IMAGE_S16 )
                {//case of input: VX_DF_IMAGE_S16
                    vx_int16 *src_ptr = (vx_int16*)src_void_ptr;
                    vx_uint8 *dst_ptr = (vx_uint8*)dst_void_ptr;

                    vx_uint8 dst_value = 0;

                    if (type == VX_THRESHOLD_TYPE_BINARY)
                    {
                        if (*src_ptr > _threshold_s16)
                            dst_value = true_value_u1  ? 1 : 0;
                        else
                            dst_value = false_value_u1 ? 1 : 0;

                        vx_uint8 offset = xShftd % 8;
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | (dst_value << offset);
                    }

                    if (type == VX_THRESHOLD_TYPE_RANGE)
                    {
                        if (*src_ptr > _upper_threshold_s16)
                            dst_value = false_value_u1 ? 1 : 0;
                        else if (*src_ptr < _lower_threshold_s16)
                            dst_value = false_value_u1 ? 1 : 0;
                        else
                            dst_value = true_value_u1  ? 1 : 0;

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
                        if (*src_ptr > _threshold_u8)
                            *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((true_value_u1  ? 1 : 0) << offset);
                        else
                            *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value_u1 ? 1 : 0) << offset);
                    }

                    if (type == VX_THRESHOLD_TYPE_RANGE)
                    {
                        vx_uint8 offset = xShftd % 8;
                        if (*src_ptr > _upper_threshold_u8)
                            *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value_u1 ? 1 : 0) << offset);
                        else if (*src_ptr < _lower_threshold_u8)
                            *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value_u1 ? 1 : 0) << offset);
                        else
                            *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((true_value_u1  ? 1 : 0) << offset);
                    }
                }
            }
        }
    }
}
