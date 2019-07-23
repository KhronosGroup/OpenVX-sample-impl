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

#include <c_model.h>

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

	status = vxGetValidRegionImage(img1, &rect);
	status |= vxAccessImagePatch(img1, &rect, 0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY);
	status |= vxCopyScalar(alpha, &scale, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
	status |= vxAccessImagePatch(img2, &rect, 0, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY);
	status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
	width = src_addr[0].dim_x;
	height = src_addr[0].dim_y;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			void *src0p = vxFormatImagePatchAddress2d(src_base[0], x, y, &src_addr[0]);
			void *src1p = vxFormatImagePatchAddress2d(src_base[1], x, y, &src_addr[1]);
			void *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
			vx_int32 src0 = *(vx_uint8 *)src0p;
			vx_int32 src1 = *(vx_uint8 *)src1p;
			vx_int32 result = (vx_int32)((1 - scale) * (vx_float32)src1 + scale * (vx_float32)src0);
			*(vx_uint8 *)dstp = (vx_uint8)result;
		}
	}
	status |= vxCommitImagePatch(img1, NULL, 0, &src_addr[0], src_base[0]);
	status |= vxCommitImagePatch(img2, NULL, 0, &src_addr[1], src_base[1]);
	status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);
	return status;
}

