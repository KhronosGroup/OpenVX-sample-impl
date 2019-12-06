/*

* Copyright (c) 2017-2017 The Khronos Group Inc.
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

void WeightedAverage_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
	vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_float32 *scale = (vx_float32*)parameters[1];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];    
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
    
    for (y = low_height; y < height; y++)
    {
		float32x4_t scale_f = vdupq_n_f32(*scale);
		float32x4_t scale_f1 = vsubq_f32(vdupq_n_f32(1.0), scale_f);
		vx_uint8* src0p = (vx_uint8 *)in_1->base[0] + in_1->tile_x + y * in_1->addr[0].stride_y;//y * src_addr[0].stride_y;
		vx_uint8* src1p = (vx_uint8 *)in_2->base[0] + in_2->tile_x + y * in_2->addr[0].stride_y;//src_addr[1].stride_y;
		vx_uint8* dstp = (vx_uint8 *)out->base[0] + out->tile_x + y * out->addr[0].stride_y;// + y * dst_addr.stride_y;
        for (x = 0; x < out->tile_block.width; x += 8)
        {
			int32x4_t src01;
            int32x4_t src02;
            int32x4_t src11;
            int32x4_t src12;
			
			uint8x8_t src0_8x8 = vld1_u8(src0p);
			uint16x8_t src0_16x8 = vmovl_u8 (src0_8x8);
			int32x4x2_t src0_32x4x2 =
			{
				{
					vreinterpretq_s32_u32 (vmovl_u16 (vget_low_u16(src0_16x8))),
					vreinterpretq_s32_u32 (vmovl_u16 (vget_high_u16(src0_16x8)))
				}
			};
			src01 = src0_32x4x2.val[0];
			src02 = src0_32x4x2.val[1];
			
			uint8x8_t src1_8x8_data = vld1_u8(src1p);
			uint16x8_t src1_16x8 = vmovl_u8 (src1_8x8_data);
			int32x4x2_t src1_32x4x2 =
			{
				{
					vreinterpretq_s32_u32 (vmovl_u16 (vget_low_u16(src1_16x8))),
					vreinterpretq_s32_u32 (vmovl_u16 (vget_high_u16(src1_16x8)))
				}
			};
			src11 = src1_32x4x2.val[0];
			src12 = src1_32x4x2.val[1];
			
			int32x4_t result1 = vcvtq_s32_f32(vaddq_f32(vmulq_f32(scale_f1, vcvtq_f32_s32(src11)), vmulq_f32(scale_f, vcvtq_f32_s32(src01))));
			int32x4_t result2 = vcvtq_s32_f32(vaddq_f32(vmulq_f32(scale_f1, vcvtq_f32_s32(src12)), vmulq_f32(scale_f, vcvtq_f32_s32(src02))));
			uint8x8_t result = vmovn_u16(vcombine_u16(vmovn_u32(vreinterpretq_u32_s32 (result1)),vmovn_u32(vreinterpretq_u32_s32 (result2))));
			vst1_u8(dstp, result);
			src0p += 8 * in_1->addr[0].stride_x;
			src1p += 8 * in_2->addr[0].stride_x;
			dstp += 8 * out->addr[0].stride_x;
		}
	}
}

#define WEIGHTEDAVERAGE_FLEXIBLE(low_y, low_x, high_y, high_x, in_1_tile_x, in_2_tile_x, out_tile_x)     \
    for (y = low_y; y < high_y; y++)                                                                     \
	{                                                                                                    \
		vx_uint8* src0p = (vx_uint8 *)in_1->base[0] + in_1_tile_x + y * in_1->addr[0].stride_y;         \
		vx_uint8* src1p = (vx_uint8 *)in_2->base[0] + in_2_tile_x + y * in_2->addr[0].stride_y;         \
		vx_uint8* dstp = (vx_uint8 *)out->base[0] + out_tile_x + y * out->addr[0].stride_y;             \
		for (x = low_x; x < high_x; x++)                                                                 \
		{                                                                                                \
			vx_int32 src0 = *src0p;                                                                      \
			vx_int32 src1 = *src1p;                                                                      \
			vx_int32 result = (vx_int32)((1 - *scale) * (vx_float32)src1 + *scale * (vx_float32)src0);   \
			*dstp = (vx_uint8)result;                                                                    \
			src0p += in_1->addr[0].stride_x;                                                             \
			src1p += in_2->addr[0].stride_x;                                                             \
			dstp += out->addr[0].stride_x;                                                               \
		}                                                                                                \
	}                                                                                                    \

void WeightedAverage_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
	vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_float32 *scale = (vx_float32*)parameters[1];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];
	vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;    
    if (ty == 0 && tx == 0)
    {
        WEIGHTEDAVERAGE_FLEXIBLE(0, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
    }
    else
    {
        WEIGHTEDAVERAGE_FLEXIBLE(0, tx, ty, vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
        WEIGHTEDAVERAGE_FLEXIBLE(ty, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), 0, 0, 0)
    }
}
