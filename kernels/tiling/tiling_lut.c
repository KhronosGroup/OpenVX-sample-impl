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

void TableLookup_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_array_t *lut = (vx_tile_array_t*)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t*)parameters[2];

    vx_uint8 *src_base = in->base[0];
    vx_uint8 *dst_base = out->base[0];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = in->tile_y + in->tile_block.height;

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = in->tile_x + in->tile_block.width;

    vx_enum type = lut->item_type;
    vx_size count = lut->num_items;
    vx_uint32 offset = lut->offset;

    void *lut_ptr = lut->ptr;

    if (type == VX_TYPE_UINT8)
    {
        int32x4_t vOffset = vdupq_n_s32((vx_int32)offset);
        int32x4_t vCnt = vdupq_n_s32((vx_int32)count);
        int32x4_t vZero = vdupq_n_s32(0);

        for (y = low_y; y < high_y; y++)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + y * in->addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;
            for (x = low_x; x < high_x; x += 16)
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
        }
    }
    else if (type == VX_TYPE_INT16)
    {
        int32x4_t vOffset = vdupq_n_s32((vx_int32)offset);
        int32x4_t vCnt = vdupq_n_s32((vx_int32)count);
        int32x4_t vZero = vdupq_n_s32(0);

        vx_int16 *lut_tmp = (vx_int16 *)lut_ptr;
        for (y = low_y; y < high_y; y++)
        {
            vx_uint8 *ptr_src = (vx_uint8 *)src_base + y * in->addr->stride_y;
            vx_uint8 *ptr_dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;
            for (x = low_x; x < high_x; x += 8)
            {
                int16x8_t vSrc = vld1q_s16((vx_int16 *)(ptr_src + x * in->addr->stride_x));
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

                int16x8_t vDstOrg = vld1q_s16((vx_int16 *)(ptr_dst + x * out->addr->stride_x));
                vVal = vbslq_s16(vPredu16, vVal, vDstOrg);
                vst1q_s16((vx_int16 *)(ptr_dst + x * out->addr->stride_x), vVal);
            }
        }
    }
}

#define TABLELOOKUP(type, low_y, high_y, low_x, high_x, type_size)                 \
    for (y = low_y; y < high_y; y++)                                               \
    {                                                                              \
        type *src_ptr = (type *)src_base + y * in->addr->stride_y / type_size;     \
        type *dst_ptr = (type *)dst_base + y * out->addr->stride_y / type_size;    \
        for (x = low_x; x < high_x; x++)                                           \
        {                                                                          \
            type *lut_tmp = (type *)lut_ptr;                                       \
            vx_int32 index = (vx_int32)offset + (vx_int32)(*src_ptr);              \
            if (index >= 0 && index < (vx_int32)count)                             \
            {                                                                      \
                *dst_ptr = lut_tmp[index];                                         \
            }                                                                      \
            src_ptr++;                                                             \
            dst_ptr++;                                                             \
        }                                                                          \
    }

void TableLookup_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_array_t *lut = (vx_tile_array_t*)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t*)parameters[2];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_enum type = lut->item_type;
    vx_size count = lut->num_items;
    vx_uint32 offset = lut->offset;

    void *lut_ptr = lut->ptr;

    if (low_y == 0 && low_x == 0)
    {
        if (type == VX_TYPE_UINT8)
        {
            vx_uint8 *src_base = in->base[0] + in->tile_x;                             
            vx_uint8 *dst_base = out->base[0] + out->tile_x;                           
            TABLELOOKUP(vx_uint8, low_y, high_y, low_x, high_x, 1)
        }
        else if (type == VX_TYPE_INT16)
        {
            vx_int16 *src_base = (vx_int16 *)in->base[0] + in->tile_x;                             
            vx_int16 *dst_base = (vx_int16 *)out->base[0] + out->tile_x;                           
            TABLELOOKUP(vx_int16, low_y, high_y, low_x, high_x, 2)
        }
    }
    else
    {
        if (type == VX_TYPE_UINT8)
        {
            vx_uint8 *src_base = in->base[0] + in->tile_x;
            vx_uint8 *dst_base = out->base[0] + out->tile_x;
            TABLELOOKUP(vx_uint8, 0, low_y, low_x, high_x, 1)

            src_base = in->base[0];
            dst_base = out->base[0];
            TABLELOOKUP(vx_uint8, low_y, high_y, 0, high_x, 1)
        }
        else if (type == VX_TYPE_INT16)
        {
            vx_int16 *src_base = (vx_int16 *)in->base[0] + in->tile_x;
            vx_int16 *dst_base = (vx_int16 *)out->base[0] + out->tile_x;
            TABLELOOKUP(vx_int16, 0, low_y, low_x, high_x, 2)

            src_base = (vx_int16 *)in->base[0];
            dst_base = (vx_int16 *)out->base[0];
            TABLELOOKUP(vx_int16, low_y, high_y, 0, high_x, 2)
        }
    }
}
