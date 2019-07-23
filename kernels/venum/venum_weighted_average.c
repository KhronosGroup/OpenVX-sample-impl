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

#include <venum.h>
#include <arm_neon.h>

// nodeless version of the Weighted Average kernel
vx_status vxWeightedAverage(vx_image img1, vx_scalar alpha, vx_image img2, vx_image output)
{
	vx_uint32 y, x, width = 0, height = 0;
	vx_float32 scale = 0.0f;
	void *dst_base = NULL;
	void *src_base[2] = { NULL, NULL };
	vx_imagepatch_addressing_t dst_addr, src_addr[2];
	vx_rectangle_t rect;
	vx_df_image img1_format = 0;
	vx_df_image img2_format = 0;
	vx_df_image out_format = 0;
	vx_status status = VX_SUCCESS;

	vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
	vxQueryImage(img1, VX_IMAGE_FORMAT, &img1_format, sizeof(img1_format));
	vxQueryImage(img2, VX_IMAGE_FORMAT, &img2_format, sizeof(img2_format));

    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
	status = vxGetValidRegionImage(img1, &rect);
	status |= vxMapImagePatch(img1, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
	status |= vxCopyScalar(alpha, &scale, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
	status |= vxMapImagePatch(img2, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
	status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr[0].dim_x;
	height = src_addr[0].dim_y;
	for (y = 0; y < height; y++)
	{
		vx_uint32 roiw8 = width >= 7 ? width : 0;
		float32x4_t scale_f = vdupq_n_f32(scale);
		float32x4_t scale_f1 = vsubq_f32(vdupq_n_f32(1.0), scale_f);
		vx_uint8* src0p = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
		vx_uint8* src1p = (vx_uint8 *)src_base[1] + y * src_addr[1].stride_y;
		vx_uint8* dstp = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
		for (x = 0; x < roiw8; x += 8)
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
			src0p += 8 * src_addr[0].stride_x;
			src1p += 8 * src_addr[1].stride_x;
			dstp += 8 * dst_addr.stride_x;
		}
		for (; x < width; x++)
		{
			vx_int32 src0 = *src0p;
			vx_int32 src1 = *src1p;
			vx_int32 result = (vx_int32)((1 - scale) * (vx_float32)src1 + scale * (vx_float32)src0);
			*dstp = (vx_uint8)result;
			
			src0p += src_addr[0].stride_x;
			src1p += src_addr[1].stride_x;
			dstp += dst_addr.stride_x;
		}
	}
    status |= vxUnmapImagePatch(img1, map_id_0);
    status |= vxUnmapImagePatch(img2, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);
	return status;
}

