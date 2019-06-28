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

static inline void vxAcc(uint8_t * input, int16_t * accum)
{
    uint8x16_t ta1 = vld1q_u8(input);
    int16x8_t  ta2 = vld1q_s16(accum);
    int16x8_t  ta3 = vld1q_s16(accum + 8);

    ta2 = vqaddq_s16(ta2, vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(ta1))));
    ta3 = vqaddq_s16(ta3, vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(ta1))));

    vst1q_s16(accum, ta2);
    vst1q_s16(accum + 8, ta3);
}


 // nodeless version of the Accumulate kernel
vx_status vxAccumulate(vx_image input, vx_image accum)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(input, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(accum, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr.dim_x;
    height = src_addr.dim_y;
    for (y = 0; y < height; y++)
    {
        vx_uint8* srcp = (uint8_t *)src_base + y * width;
        vx_int16* dstp = (vx_int16 *)dst_base + y * width;

        for (x = 0; x < width; x+=16)
        {
            vxAcc(srcp,dstp);
            srcp += 16;
            dstp += 16;
        }
    }
    status |= vxUnmapImagePatch(input, map_id);
    status |= vxUnmapImagePatch(accum, result_map_id);

    return status;
}

static inline float32x4x4_t vxConvert_u8x16_to_f32x4x4(uint8x16_t input)
{
    uint16x8_t u16_output_low = vmovl_u8(vget_low_u8(input));
    uint16x8_t u16_output_hi = vmovl_u8(vget_high_u8(input));

    float32x4x4_t res =
    {
        {
            vcvtq_f32_u32(vmovl_u16(vget_low_u16(u16_output_low))),
            vcvtq_f32_u32(vmovl_u16(vget_high_u16(u16_output_low))),
            vcvtq_f32_u32(vmovl_u16(vget_low_u16(u16_output_hi))),
            vcvtq_f32_u32(vmovl_u16(vget_high_u16(u16_output_hi)))
        }
    };

    return res;
}
static inline uint8x16_t vxConvert_f32x4x4_to_u8x16(const float32x4x4_t *input)
{
    return vcombine_u8(vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(input->val[0])),
           vmovn_u32(vcvtq_u32_f32(input->val[1])))),
           vmovn_u16(vcombine_u16(vmovn_u32(vcvtq_u32_f32(input->val[2])),
           vmovn_u32(vcvtq_u32_f32(input->val[3])))));
}

static inline float32x4x4_t vxVectorAccumulateWeighted(const float32x4x4_t *vector_input, float32x4x4_t vector_output, float32x4_t scale_val, float32x4_t scale_val2)
{
    vector_output.val[0] = vmulq_f32(vector_output.val[0], scale_val);
    vector_output.val[1] = vmulq_f32(vector_output.val[1], scale_val);
    vector_output.val[2] = vmulq_f32(vector_output.val[2], scale_val);
    vector_output.val[3] = vmulq_f32(vector_output.val[3], scale_val);

    vector_output.val[0] = vmlaq_f32(vector_output.val[0], vector_input->val[0], scale_val2);
    vector_output.val[1] = vmlaq_f32(vector_output.val[1], vector_input->val[1], scale_val2);
    vector_output.val[2] = vmlaq_f32(vector_output.val[2], vector_input->val[2], scale_val2);
    vector_output.val[3] = vmlaq_f32(vector_output.val[3], vector_input->val[3], scale_val2);

    return vector_output;
}

static inline void vxAccWe(uint8_t*  input, uint8_t*  accum, const float32x4_t scale_val, const float32x4_t scale_val2)
{
    uint8x16_t input_buffer = vld1q_u8(input);
    uint8x16_t accum_buffer = vld1q_u8(accum);

    float32x4x4_t f32_input_0 = vxConvert_u8x16_to_f32x4x4(input_buffer);
    float32x4x4_t f32_output_0 = vxConvert_u8x16_to_f32x4x4(accum_buffer);

    float32x4x4_t f32_res_0 = vxVectorAccumulateWeighted(&f32_input_0, f32_output_0, scale_val, scale_val2);

    vst1q_u8(accum, vxConvert_f32x4x4_to_u8x16(&f32_res_0));
}


// nodeless version of the AccumulateWeighted kernel
vx_status vxAccumulateWeighted(vx_image input, vx_scalar scalar, vx_image accum)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_float32 alpha = 0.0f;
    vx_status status  = VX_SUCCESS;

    vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
    vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
    rect.start_x = rect.start_y = 0;
    rect.end_x = width;
    rect.end_y = height;
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(accum, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status |= vxCopyScalar(scalar, &alpha, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    float32x4_t scale_val = vdupq_n_f32(1.f - alpha);
    float32x4_t scale_val2 = vdupq_n_f32(alpha);

    for (y = 0; y < height; y++)
    {
        uint8_t* srcp = (uint8_t *)src_base + y * width;
        uint8_t* dstp = (uint8_t *)dst_base + y * width;

        for (x = 0; x < width; x+=16)
        {
            vxAccWe(srcp, dstp, scale_val, scale_val2);
            srcp+=16;
            dstp+=16;
        }
    }
    status |= vxUnmapImagePatch(input, map_id);
    status |= vxUnmapImagePatch(accum, result_map_id);

    return status;
}
static inline void vxAccSq(uint8_t *input, uint32_t shift, int16_t *accum)
{
    uint16x8_t max_int_u16 = vdupq_n_u16((uint16_t)(INT16_MAX));
    const uint8x16_t ta1 = vld1q_u8(input);
    uint16x8_t       ta2 = vreinterpretq_u16_s16(vld1q_s16(accum));
    uint16x8_t       ta3 = vreinterpretq_u16_s16(vld1q_s16(accum + 8));

    const int16x8_t vector_shift = vdupq_n_s16(-(int16_t)shift);

    uint16x8_t linput = vmovl_u8(vget_low_u8(ta1));
    uint16x8_t hinput = vmovl_u8(vget_high_u8(ta1));

    linput = vmulq_u16(linput, linput);
    hinput = vmulq_u16(hinput, hinput);

    linput = vqshlq_u16(linput, vector_shift);
    hinput = vqshlq_u16(hinput, vector_shift);

    ta2 = vqaddq_u16(ta2, linput);
    ta3 = vqaddq_u16(ta3, hinput);

    vst1q_s16(accum, vreinterpretq_s16_u16(vminq_u16(max_int_u16, ta2)));
    vst1q_s16(accum + 8, vreinterpretq_s16_u16(vminq_u16(max_int_u16, ta3)));
}

// nodeless version of the AccumulateSquare kernel
vx_status vxAccumulateSquare(vx_image input, vx_scalar scalar, vx_image accum)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_uint32 shift = 0u;
    vx_status status = VX_SUCCESS;

    vxCopyScalar(scalar, &shift, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status = vxGetValidRegionImage(input, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(accum, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr.dim_x;
    height = src_addr.dim_y;
    for (y = 0; y < height; y++)
    {
        vx_uint8* srcp = (vx_uint8 *)src_base + y * width;
        vx_int16 *dstp = (vx_int16 *)dst_base + y * width;
        for (x = 0; x < width; x+=16)
        {
            vxAccSq(srcp, shift, dstp);
            srcp += 16;
            dstp += 16;
        }
    }
    status |= vxUnmapImagePatch(input, map_id);
    status |= vxUnmapImagePatch(accum, result_map_id);

    return status;
}