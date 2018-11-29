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
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, (void **)&src_base,VX_READ_AND_WRITE);
    status |= vxAccessImagePatch(accum, &rect, 0, &dst_addr, (void **)&dst_base,VX_READ_AND_WRITE);
    width = src_addr.dim_x;
    height = src_addr.dim_y;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            vx_uint8 *srcp = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            vx_int16 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            vx_int32 res = ((vx_int32)(*dstp)) + (vx_int32)(*srcp);
            if (res > INT16_MAX) // saturate to S16
                res = INT16_MAX;
            *dstp = (vx_int16)(res);
        }
    }
    status |= vxCommitImagePatch(input, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(accum, &rect, 0, &dst_addr, dst_base);

    return status;
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
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, (void **)&src_base,VX_READ_AND_WRITE);
    status |= vxAccessImagePatch(accum, &rect, 0, &dst_addr, (void **)&dst_base,VX_READ_AND_WRITE);
    status |= vxCopyScalar(scalar, &alpha, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            vx_uint8 *srcp = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            *dstp = (vx_uint8)(((1 - alpha) * (*dstp)) + ((alpha) * (vx_uint16)(*srcp)));
        }
    }
    status |= vxCommitImagePatch(input, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(accum, &rect, 0, &dst_addr, dst_base);

    return status;
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
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, (void **)&src_base,VX_READ_AND_WRITE);
    status |= vxAccessImagePatch(accum, &rect, 0, &dst_addr, (void **)&dst_base,VX_READ_AND_WRITE);
    width = src_addr.dim_x;
    height = src_addr.dim_y;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            vx_uint8 *srcp = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            vx_int16 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            vx_int32 res = ((vx_int32)(*srcp) * (vx_int32)(*srcp));
            res = ((vx_uint32)*dstp) + (res >> shift);
            if (res > INT16_MAX) // saturate to S16
                res = INT16_MAX;
            *dstp = (vx_int16)(res);
        }
    }
    status |= vxCommitImagePatch(input, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(accum, &rect, 0, &dst_addr, dst_base);

    return status;
}

