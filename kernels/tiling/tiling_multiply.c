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

// nodeless version of the Multiply kernel
void Multiply_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_float32 *scale = (vx_float32*)parameters[2];
    vx_enum *overflow_policy = (vx_enum*)parameters[3];
    vx_enum *rounding_policy = (vx_enum*)parameters[4];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[5];    
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
    
    for (y = low_height; y < height; y++)
    {
        vx_uint8 *src0p = (vx_uint8 *)in_1->base[0] + in_1->tile_x + y * in_1->image.width;
        vx_uint8 *src1p = (vx_uint8 *)in_2->base[0] + in_2->tile_x + y * in_2->image.width;
        vx_uint8 *dstp = (vx_uint8 *)out->base[0] + out->tile_x + y * out->image.width;        
        vx_int16 *src0p_16 = (vx_int16 *)in_1->base[0] + in_1->tile_x + y * in_1->image.width;
        vx_int16 *src1p_16 = (vx_int16 *)in_2->base[0] + in_2->tile_x + y * in_2->image.width; 
        vx_int16 *dstp_16 = (vx_int16 *)out->base[0] + out->tile_x + y * out->image.width; 
        for (x = 0; x < out->tile_block.width; x += 8)
        {            
            int32x4_t src01;
            int32x4_t src02;
            int32x4_t src11;
            int32x4_t src12;
            if(in_1->image.format == VX_DF_IMAGE_U8)
            {
                uint8x8_t in01_8x8_data = vld1_u8((vx_uint8*)src0p);
                uint16x8_t tmp16x8 = vmovl_u8 (in01_8x8_data);
                int32x4x2_t tmp32x4_int_u8 =
                {
                    {
                        vreinterpretq_s32_u32 (vmovl_u16 (vget_low_u16(tmp16x8))),
                        vreinterpretq_s32_u32 (vmovl_u16 (vget_high_u16(tmp16x8)))
                    }
                };
                src01 = tmp32x4_int_u8.val[0];
                src02 = tmp32x4_int_u8.val[1];
                src0p += 8;
            }
            else
            {
                int16x8_t int02_16x8_data = vld1q_s16((vx_int16*)src0p_16);
                int32x4x2_t tmp32x4_int_s16 =
                {
                    {
                        vmovl_s16 (vget_low_s16(int02_16x8_data)),
                        vmovl_s16 (vget_high_s16(int02_16x8_data))
                    }
                };
                src01 = tmp32x4_int_s16.val[0];
                src02 = tmp32x4_int_s16.val[1];
                src0p_16 += 8;
            }            
            if(in_2->image.format == VX_DF_IMAGE_U8)
            {
                uint8x8_t in01_8x8_data = vld1_u8((vx_uint8*)src1p);
                uint16x8_t tmp16x8 = vmovl_u8 (in01_8x8_data);
                int32x4x2_t tmp32x4_int_u8 =
                {
                    {
                        vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(tmp16x8))),
                        vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(tmp16x8)))
                    }
                };
                src11 = tmp32x4_int_u8.val[0];
                src12 = tmp32x4_int_u8.val[1];
                src1p += 8;
            }
            else
            {
                int16x8_t int02_16x8_data = vld1q_s16((vx_int16*)src1p_16);
                int32x4x2_t tmp32x4_int_s16 =
                {
                    {
                        vmovl_s16(vget_low_s16(int02_16x8_data)),
                        vmovl_s16(vget_high_s16(int02_16x8_data))
                    }
                };
                src11 = tmp32x4_int_s16.val[0];
                src12 = tmp32x4_int_s16.val[1];
                src1p_16 += 8;
            }
            int32x4_t unscaled_unconverted_result1 = vmulq_s32(src01, src11);
            int32x4_t unscaled_unconverted_result2 = vmulq_s32(src02, src12);
            vx_int32 tmp0 = vgetq_lane_s32(unscaled_unconverted_result1, 0);
            vx_int32 tmp1 = vgetq_lane_s32(unscaled_unconverted_result1, 1);
            vx_int32 tmp2 = vgetq_lane_s32(unscaled_unconverted_result1, 2);
            vx_int32 tmp3 = vgetq_lane_s32(unscaled_unconverted_result1, 3);
            vx_int32 tmp4 = vgetq_lane_s32(unscaled_unconverted_result2, 0);
            vx_int32 tmp5 = vgetq_lane_s32(unscaled_unconverted_result2, 1);
            vx_int32 tmp6 = vgetq_lane_s32(unscaled_unconverted_result2, 2);
            vx_int32 tmp7 = vgetq_lane_s32(unscaled_unconverted_result2, 3);
               
            vx_int32 i;
            for(i = 0; i < 8; i++)
            {   
                vx_int32 tmp_int32;
                if(i == 0)
                  tmp_int32 = tmp0;
                else if(i == 1)
                  tmp_int32 = tmp1;
                else if(i == 2)
                  tmp_int32 = tmp2;
                else if(i == 3)
                  tmp_int32 = tmp3;
                else if(i == 4)
                  tmp_int32 = tmp4;
                else if(i == 5)
                  tmp_int32 = tmp5;
                else if(i == 6)
                  tmp_int32 = tmp6;
                else if(i == 7)
                  tmp_int32 = tmp7;
                vx_float64 unscaled_result = (vx_float64)tmp_int32;
                vx_float64 scaled_result = (*scale) * unscaled_result;
                vx_int32 int_typed_result = (vx_int32)scaled_result;
                vx_int32 final_result_value;
                if (*overflow_policy == VX_CONVERT_POLICY_SATURATE)
                {
                    if (out->image.format == VX_DF_IMAGE_U8)
                    {
                        if (int_typed_result > UINT8_MAX)
                            final_result_value = UINT8_MAX;
                        else if (int_typed_result < 0)
                            final_result_value = 0;
                        else
                            final_result_value = int_typed_result;
                    }
                    else 
                    {
                        if (int_typed_result > INT16_MAX)
                            final_result_value = INT16_MAX;
                        else if (int_typed_result < INT16_MIN)
                            final_result_value = INT16_MIN;
                        else
                            final_result_value = int_typed_result;
                    }
                }
                else 
                {
                    final_result_value = (out->image.format == VX_DF_IMAGE_U8) ?
                        (vx_uint8)int_typed_result : (vx_int16)int_typed_result;
                }

                if (out->image.format == VX_DF_IMAGE_U8)
                {
                    *dstp = (vx_uint8)final_result_value;
                    dstp += 1;
                }
                else
                {
                    *dstp_16 = (vx_int16)final_result_value;
                    dstp_16 += 1;
                }
            }
        }
    }
}

#define MULTIPLY_FLEXIBLE(low_y, low_x, high_y, high_x, in_1_tile_x, in_2_tile_x, out_tile_x) \
    for (y = low_y; y < high_y; y++)                                                           \
    {                                                                                          \
        vx_uint8 *src0p = (vx_uint8 *)in_1->base[0] + in_1_tile_x + y * in_1->image.width;    \
        vx_uint8 *src1p = (vx_uint8 *)in_2->base[0] + in_2_tile_x + y * in_2->image.width;    \
        vx_uint8 *dstp = (vx_uint8 *)out->base[0] + out_tile_x + y * out->image.width;        \
        vx_int16 *src0p_16 = (vx_int16 *)in_1->base[0] + in_1_tile_x + y * in_1->image.width; \
        vx_int16 *src1p_16 = (vx_int16 *)in_2->base[0] + in_2_tile_x + y * in_2->image.width; \
        vx_int16 *dstp_16 = (vx_int16 *)out->base[0] + out_tile_x + y * out->image.width;     \
        for (x = low_x; x < high_x; x++)                                                       \
        {                                                                                      \
            vx_int32 src0 = in_1->image.format == VX_DF_IMAGE_U8 ? *src0p : *src0p_16;         \
            vx_int32 src1 = in_2->image.format == VX_DF_IMAGE_U8 ? *src1p : *src1p_16;         \
            src0p++;                                                                           \
            src1p++;                                                                           \
            src0p_16++;                                                                        \
            src1p_16++;                                                                        \
            vx_int32 unscaled_unconverted_result = src0 * src1;                                \
            vx_float64 unscaled_result = (vx_float64)unscaled_unconverted_result;              \
            vx_float64 scaled_result = (*scale) * unscaled_result;                             \
            vx_int32 int_typed_result = (vx_int32)scaled_result;                               \
            vx_int32 final_result_value;                                                       \
            if (*overflow_policy == VX_CONVERT_POLICY_SATURATE)                                \
            {                                                                                  \
                if (out->image.format == VX_DF_IMAGE_U8)                                       \
                {                                                                              \
                    if (int_typed_result > UINT8_MAX)                                          \
                        final_result_value = UINT8_MAX;                                        \
                    else if (int_typed_result < 0)                                             \
                        final_result_value = 0;                                                \
                    else                                                                       \
                        final_result_value = int_typed_result;                                 \
                }                                                                              \
                else                                                                           \
                {                                                                              \
                    if (int_typed_result > INT16_MAX)                                          \
                        final_result_value = INT16_MAX;                                        \
                    else if (int_typed_result < INT16_MIN)                                     \
                        final_result_value = INT16_MIN;                                        \
                    else                                                                       \
                        final_result_value = int_typed_result;                                 \
                }                                                                              \
            }                                                                                  \
            else                                                                               \
            {                                                                                  \
                final_result_value = (out->image.format == VX_DF_IMAGE_U8) ?                   \
                    (vx_uint8)int_typed_result : (vx_int16)int_typed_result;                   \
            }                                                                                  \
            if (out->image.format == VX_DF_IMAGE_U8)                                           \
            {                                                                                  \
                *dstp = (vx_uint8)final_result_value;                                          \
                dstp++;                                                                        \
            }                                                                                  \
            else                                                                               \
            {                                                                                  \
                *dstp_16 = (vx_int16)final_result_value;                                       \
                dstp_16++;                                                                     \
            }                                                                                  \
        }                                                                                      \
    }                                                                                          
    
    
void Multiply_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_float32 *scale = (vx_float32*)parameters[2];
    vx_enum *overflow_policy = (vx_enum*)parameters[3];
    vx_enum *rounding_policy = (vx_enum*)parameters[4];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[5];
    
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;
    if (ty == 0 && tx == 0)
    {
        MULTIPLY_FLEXIBLE(0, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)     
    }
    else
    {
        MULTIPLY_FLEXIBLE(0, tx, ty, vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
        MULTIPLY_FLEXIBLE(ty, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), 0, 0, 0)
    }
}

