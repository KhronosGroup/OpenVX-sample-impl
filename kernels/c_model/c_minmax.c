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

typedef vx_int32 (arithmeticOp)(vx_int32, vx_int32);

static vx_int32 vx_min_op(vx_int32 a, vx_int32 b)
{
    if (a <= b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

static vx_int32 vx_max_op(vx_int32 a, vx_int32 b)
{
    if (a >= b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

// generic arithmetic op
static vx_status vxBinaryU8S16OverflowOp(vx_image in0, vx_image in1, vx_image output, arithmeticOp op)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_df_image in0_format = 0;
    vx_df_image in1_format = 0;
    vx_df_image out_format = 0;
    vx_status status = VX_SUCCESS;

    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(in0, VX_IMAGE_FORMAT, &in0_format, sizeof(in0_format));
    vxQueryImage(in1, VX_IMAGE_FORMAT, &in1_format, sizeof(in1_format));

    status = vxGetValidRegionImage(in0, &rect);
    status |= vxAccessImagePatch(in0, &rect, 0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY);
    status |= vxAccessImagePatch(in1, &rect, 0, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY);
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
            vx_int32 src0 = in0_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)src0p : *(vx_int16 *)src0p;
            vx_int32 src1 = in1_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)src1p : *(vx_int16 *)src1p;
            vx_int32 result = op(src0, src1);
            if (out_format == VX_DF_IMAGE_U8)
            {
                *(vx_uint8 *)dstp = (vx_uint8)result;
            }
            else
            {
                *(vx_int16 *)dstp = (vx_int16)result;
            }
        }
    }
    status |= vxCommitImagePatch(in0, NULL, 0, &src_addr[0], src_base[0]);
    status |= vxCommitImagePatch(in1, NULL, 0, &src_addr[1], src_base[1]);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);
    return status;
}

// nodeless version of the Min kernel
vx_status vxMin(vx_image in0, vx_image in1, vx_image output)
{
    return vxBinaryU8S16OverflowOp(in0, in1, output, vx_min_op);
}

// nodeless version of the Max kernel
vx_status vxMax(vx_image in0, vx_image in1, vx_image output)
{
    return vxBinaryU8S16OverflowOp(in0, in1, output, vx_max_op);
}
