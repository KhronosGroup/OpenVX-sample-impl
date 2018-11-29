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

#include <c_model.h>

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
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    status |= vxAccessLUT(lut, &lut_ptr, VX_READ_ONLY);

    for (y = 0; (y < src_addr.dim_y) && (status == VX_SUCCESS); y++)
    {
        for (x = 0; x < src_addr.dim_x; x++)
        {
            if (type == VX_TYPE_UINT8)
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
            else if (type == VX_TYPE_INT16)
            {
                vx_int16 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                vx_int16 *dst_ptr = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_int16 *lut_tmp = (vx_int16 *)lut_ptr;
                vx_int32 index = (vx_int32)offset + (vx_int32)(*src_ptr);
                if (index >= 0 && index < (vx_int32)count)
                {
                    *dst_ptr = lut_tmp[index];
                }
            }
        }
    }

    status |= vxCommitLUT(lut, lut_ptr);
    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

    return status;
}

