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

// nodeless version of the Multiply kernel
vx_status vxMultiply(vx_image in0, vx_image in1, vx_scalar scale_param, vx_scalar opolicy_param, vx_scalar rpolicy_param, vx_image output)
{
    vx_float32 scale = 0.0f;
    vx_enum overflow_policy = -1;
    vx_enum rounding_policy = -1;
    vx_uint32 y, x;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_df_image in0_format = 0;
    vx_df_image in1_format = 0;
    vx_df_image out_format = 0;
    vx_status status = VX_FAILURE;

    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(in0, VX_IMAGE_FORMAT, &in0_format, sizeof(in0_format));
    vxQueryImage(in1, VX_IMAGE_FORMAT, &in1_format, sizeof(in1_format));

    status = vxGetValidRegionImage(in0, &rect);
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in0, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxCopyScalar(scale_param, &scale, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(opolicy_param, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(rpolicy_param, &rounding_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    for (y = 0; y < dst_addr.dim_y; y++)
    {
        vx_uint32 roiw8 = dst_addr.dim_x >= 7 ? dst_addr.dim_x - 7 : 0;
        for (x = 0; x < roiw8;x += 8)
        {
            void *src0p = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
            void *src1p = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
            void *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            int32x4_t src01;
            int32x4_t src02;
            int32x4_t src11;
            int32x4_t src12;
            if(in0_format == VX_DF_IMAGE_U8)
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
            }
            else
            {
                int16x8_t int02_16x8_data = vld1q_s16((vx_int16*)src0p);
                int32x4x2_t tmp32x4_int_s16 =
                {
                    {
                        vmovl_s16 (vget_low_s16(int02_16x8_data)),
                        vmovl_s16 (vget_high_s16(int02_16x8_data))
                    }
                };
                src01 = tmp32x4_int_s16.val[0];
                src02 = tmp32x4_int_s16.val[1];
            }
            
            if(in1_format == VX_DF_IMAGE_U8)
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
            }
            else
            {
                int16x8_t int02_16x8_data = vld1q_s16((vx_int16*)src1p);
                int32x4x2_t tmp32x4_int_s16 =
                {
                    {
                        vmovl_s16(vget_low_s16(int02_16x8_data)),
                        vmovl_s16(vget_high_s16(int02_16x8_data))
                    }
                };
                src11 = tmp32x4_int_s16.val[0];
                src12 = tmp32x4_int_s16.val[1];
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
                vx_float64 scaled_result = scale * unscaled_result;
                vx_int32 int_typed_result = (vx_int32)scaled_result;
                vx_int32 final_result_value;
                if (overflow_policy == VX_CONVERT_POLICY_SATURATE)
                {
                    if (out_format == VX_DF_IMAGE_U8)
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
                    final_result_value = (out_format == VX_DF_IMAGE_U8) ?
                        (vx_uint8)int_typed_result : (vx_int16)int_typed_result;
                }

                if (out_format == VX_DF_IMAGE_U8)
                {
                  *(vx_uint8 *)dstp = (vx_uint8)final_result_value;
                  dstp += 1;
                }
                else
                {
                  *(vx_int16 *)dstp = (vx_int16)final_result_value;
                  dstp += 2;
                }
            }
        }
    }
    status |= vxUnmapImagePatch(in0, map_id_0);
    status |= vxUnmapImagePatch(in1, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);
    return status;
}

