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
#include "tensor_utils.h"

// nodeless version of the TableLookup kernel
vx_status vxTensorTableLookup(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims, void* lut, vx_size lut_size,
        vx_uint32 lut_offset, void* dst, vx_size* dst_strides, vx_enum type)
{
    vx_status status = VX_SUCCESS;
    vx_size out_size = ComputeNumberOfElements(dims, num_of_dims);
    vx_int32 roiw8 = out_size >= 7 ? out_size - 7 : 0;
    vx_size i = 0;
    for (; i < roiw8; i+=8)
    {
        vx_size src_pos = ComputeGlobalPositionsFromIndex(i, dims, src_strides, num_of_dims, &src_pos);
        vx_size dst_pos = ComputeGlobalPositionsFromIndex(i, dims, dst_strides, num_of_dims, &dst_pos);
        if (type == VX_TYPE_UINT8)
        {
            vx_uint8 *src_ptr = src+src_pos;
            vx_uint8 *dst_ptr = dst+dst_pos;
            vx_uint8 *lut_tmp = (vx_uint8 *)lut;
            
            uint8x8_t src_u8x8 = vld1_u8(src_ptr);
            uint16x8_t src_u16x8 = vmovl_u8(src_u8x8);
            int32x4x2_t src_s32x4 = 
            {
                {
                  vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(src_u16x8))),
                  vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(src_u16x8)))
                 }
            };
            int32x4_t lut_offset_s32x4 = vdupq_n_s32((vx_int32)lut_offset);
            int32x4_t index_s32x4_0 = vaddq_s32(lut_offset_s32x4, src_s32x4.val[0]);
            int32x4_t index_s32x4_1 = vaddq_s32(lut_offset_s32x4, src_s32x4.val[1]);
            vx_int32 lut_size_s32 = (vx_int32)lut_size;
            if(vgetq_lane_s32(index_s32x4_0, 0) >= 0 && vgetq_lane_s32(index_s32x4_0, 0) < lut_size_s32)
            {
                *dst_ptr = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 0));
            }
            if(vgetq_lane_s32(index_s32x4_0, 1) >= 0 && vgetq_lane_s32(index_s32x4_0, 1) < lut_size_s32)
            {
                *(dst_ptr+1) = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 1));
            }
            if(vgetq_lane_s32(index_s32x4_0, 2) >= 0 && vgetq_lane_s32(index_s32x4_0, 2) < lut_size_s32)
            {
                *(dst_ptr+2) = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 2));
            }
            if(vgetq_lane_s32(index_s32x4_0, 3) >= 0 && vgetq_lane_s32(index_s32x4_0, 3) < lut_size_s32)
            {
                *(dst_ptr+3) = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 3));
            }
            if(vgetq_lane_s32(index_s32x4_1, 0) >= 0 && vgetq_lane_s32(index_s32x4_1, 0) < lut_size_s32)
            {
                *(dst_ptr+4) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 0));
            }
            if(vgetq_lane_s32(index_s32x4_1, 1) >= 0 && vgetq_lane_s32(index_s32x4_1, 1) < lut_size_s32)
            {
                *(dst_ptr+5) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 1));
            }
            if(vgetq_lane_s32(index_s32x4_1, 2) >= 0 && vgetq_lane_s32(index_s32x4_1, 2) < lut_size_s32)
            {
                *(dst_ptr+6) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 2));
            }
            if(vgetq_lane_s32(index_s32x4_1, 3) >= 0 && vgetq_lane_s32(index_s32x4_1, 3) < lut_size_s32)
            {
                *(dst_ptr+7) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 3));
            }           
        }
        else if (type == VX_TYPE_INT16)
        {
            vx_int16 *src_ptr = src+src_pos;
            vx_int16 *dst_ptr = dst+dst_pos;
            vx_int16 *lut_tmp = (vx_int16 *)lut;            
            int16x8_t src_s16x8 = vld1q_s16(src_ptr);
            int32x4x2_t src_s32x4 = 
            {
              {
                  vmovl_s16(vget_low_s16(src_s16x8)),
                  vmovl_s16(vget_high_s16(src_s16x8))
              }
            };
            int32x4_t lut_offset_s32x4 = vdupq_n_s32((vx_int32)lut_offset);
            int32x4_t index_s32x4_0 = vaddq_s32(lut_offset_s32x4, src_s32x4.val[0]);
            int32x4_t index_s32x4_1 = vaddq_s32(lut_offset_s32x4, src_s32x4.val[1]);
            vx_int32 lut_size_s32 = (vx_int32)lut_size;
            if(vgetq_lane_s32(index_s32x4_0, 0) >= 0 && vgetq_lane_s32(index_s32x4_0, 0) < lut_size_s32)
            {
                *dst_ptr = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 0));
            }
            if(vgetq_lane_s32(index_s32x4_0, 1) >= 0 && vgetq_lane_s32(index_s32x4_0, 1) < lut_size_s32)
            {
                *(dst_ptr+1) = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 1));
            }
            if(vgetq_lane_s32(index_s32x4_0, 2) >= 0 && vgetq_lane_s32(index_s32x4_0, 2) < lut_size_s32)
            {
                *(dst_ptr+2) = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 2));
            }
            if(vgetq_lane_s32(index_s32x4_0, 3) >= 0 && vgetq_lane_s32(index_s32x4_0, 3) < lut_size_s32)
            {
                *(dst_ptr+3) = *(lut_tmp + vgetq_lane_s32(index_s32x4_0, 3));
            }
            if(vgetq_lane_s32(index_s32x4_1, 0) >= 0 && vgetq_lane_s32(index_s32x4_1, 0) < lut_size_s32)
            {
                *(dst_ptr+4) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 0));
            }
            if(vgetq_lane_s32(index_s32x4_1, 1) >= 0 && vgetq_lane_s32(index_s32x4_1, 1) < lut_size_s32)
            {
                *(dst_ptr+5) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 1));
            }
            if(vgetq_lane_s32(index_s32x4_1, 2) >= 0 && vgetq_lane_s32(index_s32x4_1, 2) < lut_size_s32)
            {
                *(dst_ptr+6) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 2));
            }
            if(vgetq_lane_s32(index_s32x4_1, 3) >= 0 && vgetq_lane_s32(index_s32x4_1, 3) < lut_size_s32)
            {
                *(dst_ptr+7) = *(lut_tmp + vgetq_lane_s32(index_s32x4_1, 3));
            }            
        }
    }
    for (; i < out_size; i++)
    {
        vx_size src_pos = ComputeGlobalPositionsFromIndex(i, dims, src_strides, num_of_dims, &src_pos);
        vx_size dst_pos = ComputeGlobalPositionsFromIndex(i, dims, dst_strides, num_of_dims, &dst_pos);
        if (type == VX_TYPE_UINT8)
        {
            vx_uint8 *src_ptr = src+src_pos;
            vx_uint8 *dst_ptr = dst+dst_pos;
            vx_uint8 *lut_tmp = (vx_uint8 *)lut;
            vx_int32 index = (vx_int32)lut_offset + (vx_int32)(*src_ptr);
            if (index >= 0 && index < (vx_int32)lut_size)
            {
                *dst_ptr = lut_tmp[index];
            }
        }
        else if (type == VX_TYPE_INT16)
        {
            vx_int16 *src_ptr = src+src_pos;
            vx_int16 *dst_ptr = dst+dst_pos;
            vx_int16 *lut_tmp = (vx_int16 *)lut;
            vx_int32 index = (vx_int32)lut_offset + (vx_int32)(*src_ptr);
            if (index >= 0 && index < (vx_int32)lut_size)
            {
                *dst_ptr = lut_tmp[index];
            }
        }
   }
    return status;
}

