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
#include <venum.h>

// nodeless version of the TableLookup kernel
vx_status vxTableLookup(vx_image src, vx_lut lut, vx_image dst)
{
    vx_enum type = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    void *src_base = NULL, *dst_base = NULL, *lut_ptr = NULL;
    vx_uint32 y = 0, x = 0;
    vx_size count = 0;
    vx_uint32 offset = 0;
    vx_status status = VX_SUCCESS;

    vxQueryLUT(lut, VX_LUT_TYPE, &type, sizeof(type));
    vxQueryLUT(lut, VX_LUT_COUNT, &count, sizeof(count));
    vxQueryLUT(lut, VX_LUT_OFFSET, &offset, sizeof(offset));
    status = vxGetValidRegionImage(src, &rect);
    vx_map_id map_id_src = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(src, &rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &map_id_dst, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxAccessLUT(lut, &lut_ptr, VX_READ_ONLY);

    if (status == VX_SUCCESS && type == VX_TYPE_UINT8)
    {
        int32x4_t vOffset = vdupq_n_s32((vx_int32)offset);
        int32x4_t vCnt = vdupq_n_s32((vx_int32)count);
        int32x4_t vZero = vdupq_n_s32(0);
        vx_uint32 w16 = (src_addr.dim_x >> 4) << 4;
        for (y = 0; y < src_addr.dim_y; y++)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + y * src_addr.stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (x = 0; x < w16; x += 16)
            {
                vx_uint8 *lut_tmp = (vx_uint8 *)lut_ptr;
                uint8x16_t vSrc = vld1q_u8(ptr_src + x);
                uint16x8_t vSrcs16_low = vmovl_u8(vget_low_u8(vSrc));
                uint16x8_t vSrcs16_high = vmovl_u8(vget_high_u8(vSrc));
                int32x4_t vPoss32_low = vaddq_s32(vOffset, vmovl_s16(vreinterpret_s16_u16(vget_low_u16(vSrcs16_low))));
                int32x4_t vPoss32_high = vaddq_s32(vOffset, vmovl_s16(vreinterpret_s16_u16(vget_high_u16(vSrcs16_low))));
                uint32x4_t vPreds32_low = vcgeq_s32(vPoss32_low, vZero);
                uint32x4_t vPreds32_tmp = vcltq_s32(vPoss32_low, vCnt);
                vPreds32_low = vandq_u32(vPreds32_low, vPreds32_tmp);
                vPoss32_low = vbslq_s32(vPreds32_low, vPoss32_low, vZero);
                uint32x4_t vPreds32_high = vcgeq_s32(vPoss32_high, vZero);
                vPreds32_tmp = vcltq_s32(vPoss32_high, vCnt);
                vPreds32_high = vandq_u32(vPreds32_high, vPreds32_tmp);
                vPoss32_high = vbslq_s32(vPreds32_high, vPoss32_high, vZero);
                uint8x8_t vPredu8_low = vmovn_u16(vcombine_u16(vmovn_u32(vPreds32_low), vmovn_u32(vPreds32_high)));

                uint8x16_t vVal = vdupq_n_u8(0);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 0)], vVal, 0);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 1)], vVal, 1);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 2)], vVal, 2);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 3)], vVal, 3);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 0)], vVal, 4);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 1)], vVal, 5);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 2)], vVal, 6);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 3)], vVal, 7);

                vPoss32_low = vaddq_s32(vOffset, vmovl_s16(vreinterpret_s16_u16(vget_low_u16(vSrcs16_high))));
                vPoss32_high = vaddq_s32(vOffset, vmovl_s16(vreinterpret_s16_u16(vget_high_u16(vSrcs16_high))));
                vPreds32_low = vcgeq_s32(vPoss32_low, vZero);
                vPreds32_tmp = vcltq_s32(vPoss32_low, vCnt);
                vPreds32_low = vandq_u32(vPreds32_low, vPreds32_tmp);
                vPoss32_low = vbslq_s32(vPreds32_low, vPoss32_low, vZero);
                vPreds32_high = vcgeq_s32(vPoss32_high, vZero);
                vPreds32_tmp = vcltq_s32(vPoss32_high, vCnt);
                vPreds32_high = vandq_u32(vPreds32_high, vPreds32_tmp);
                vPoss32_high = vbslq_s32(vPreds32_high, vPoss32_high, vZero);
                uint8x8_t vPredu8_high = vmovn_u16(vcombine_u16(vmovn_u32(vPreds32_low), vmovn_u32(vPreds32_high)));

                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 0)], vVal, 8);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 1)], vVal, 9);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 2)], vVal, 10);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_low, 3)], vVal, 11);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 0)], vVal, 12);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 1)], vVal, 13);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 2)], vVal, 14);
                vVal = vsetq_lane_u8(lut_tmp[vgetq_lane_s32(vPoss32_high, 3)], vVal, 15);

                uint8x16_t vPredu8 = vcombine_u8(vPredu8_low, vPredu8_high);
                uint8x16_t vDstOrg = vld1q_u8(ptr_dst + x);
                vVal = vbslq_u8(vPredu8, vVal, vDstOrg);
                vst1q_u8(ptr_dst + x, vVal);
            }
            for (x = w16; x < src_addr.dim_x; x++)
            {
                vx_uint8 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_uint8 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_uint8 *lut_tmp = (vx_uint8 *)lut_ptr;
                vx_int32 index = (vx_int32)offset + (vx_int32)(*src_ptr);
                if (index >= 0 && index < (vx_int32)count)
                {
                    *dst_ptr = lut_tmp[index];
                }
            }
        }
    }
    else if (status == VX_SUCCESS && type == VX_TYPE_INT16)
    {
        int32x4_t vOffset = vdupq_n_s32((vx_int32)offset);
        int32x4_t vCnt = vdupq_n_s32((vx_int32)count);
        int32x4_t vZero = vdupq_n_s32(0);
        vx_uint32 w8 = (src_addr.dim_x >> 3) << 3;
        vx_int16 *lut_tmp = (vx_int16 *)lut_ptr;
        for (y = 0; y < src_addr.dim_y; y++)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + y * src_addr.stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (x = 0; x < w8; x += 8)
            {
                int16x8_t vSrc = vld1q_s16((vx_int16 *)(ptr_src + x * src_addr.stride_x));
                int32x4_t vPoss32_low = vaddq_s32(vOffset, vmovl_s16(vget_low_s16(vSrc)));
                int32x4_t vPoss32_high = vaddq_s32(vOffset, vmovl_s16(vget_high_s16(vSrc)));
                uint32x4_t vPreds32_low = vcgeq_s32(vPoss32_low, vZero);
                uint32x4_t vPreds32_tmp = vcltq_s32(vPoss32_low, vCnt);
                vPreds32_low = vandq_u32(vPreds32_low, vPreds32_tmp);
                vPoss32_low = vbslq_s32(vPreds32_low, vPoss32_low, vZero);
                uint32x4_t vPreds32_high = vcgeq_s32(vPoss32_high, vZero);
                vPreds32_tmp = vcltq_s32(vPoss32_high, vCnt);
                vPreds32_high = vandq_u32(vPreds32_high, vPreds32_tmp);
                vPoss32_high = vbslq_s32(vPreds32_high, vPoss32_high, vZero);
                uint16x8_t vPredu16 = vcombine_u16(vmovn_u32(vPreds32_low), vmovn_u32(vPreds32_high));

                int16x8_t vVal = vdupq_n_s16(0);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_low, 0)], vVal, 0);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_low, 1)], vVal, 1);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_low, 2)], vVal, 2);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_low, 3)], vVal, 3);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_high, 0)], vVal, 4);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_high, 1)], vVal, 5);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_high, 2)], vVal, 6);
                vVal = vsetq_lane_s16(lut_tmp[vgetq_lane_s32(vPoss32_high, 3)], vVal, 7);

                int16x8_t vDstOrg = vld1q_s16((vx_int16 *)(ptr_dst + x * dst_addr.stride_x));
                vVal = vbslq_s16(vPredu16, vVal, vDstOrg);
                vst1q_s16((vx_int16 *)(ptr_dst + x * dst_addr.stride_x), vVal);
            }
            for (x = w8; x < src_addr.dim_x; x++)
            {
                vx_int16 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_int16 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_int32 index = (vx_int32)offset + (vx_int32)(*src_ptr);
                if (index >= 0 && index < (vx_int32)count)
                {
                    *dst_ptr = lut_tmp[index];
                }
            }
        }
    }
    status |= vxCommitLUT(lut, lut_ptr);
    status |= vxUnmapImagePatch(src, map_id_src);
    status |= vxUnmapImagePatch(dst, map_id_dst);
    return status;
}

