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

typedef vx_uint8 (bitwiseOp)(vx_uint8, vx_uint8);

static vx_uint8 vx_and_op(vx_uint8 a, vx_uint8 b)
{
    return a & b;
}

static vx_uint8 vx_or_op(vx_uint8 a, vx_uint8 b)
{
    return a | b;
}

static vx_uint8 vx_xor_op(vx_uint8 a, vx_uint8 b)
{
    return a ^ b;
}

// generic U1 bitwise op
static vx_status vxBinaryU1Op(vx_image in1, vx_image in2, vx_image output, bitwiseOp op)
{
    vx_uint32 y, x, width, height;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status  = vxGetValidRegionImage(in1, &rect);
    status |= vxAccessImagePatch(in1, &rect, 0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY);
    status |= vxAccessImagePatch(in2, &rect, 0, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
    width  = src_addr[0].dim_x - rect.start_x % 8;  // vxAccessImagePatch rounds start_x down to the nearest byte boundary
    height = src_addr[0].dim_y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            vx_uint32 xShftd = x + rect.start_x % 8;     // U1 valid region start bit offset
            vx_uint8 *src[2] = {
                vxFormatImagePatchAddress2d(src_base[0], xShftd, y, &src_addr[0]),
                vxFormatImagePatchAddress2d(src_base[1], xShftd, y, &src_addr[1]),
            };
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, xShftd, y, &dst_addr);

            vx_uint8 mask  = 1 << (xShftd % 8);
            vx_uint8 pixel = op(*src[0], *src[1]) & mask;
            *dst = (*dst & ~mask) | pixel;
        }
    }
    status |= vxCommitImagePatch(in1, NULL, 0, &src_addr[0], src_base[0]);
    status |= vxCommitImagePatch(in2, NULL, 0, &src_addr[1], src_base[1]);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);

    return status;
}

// nodeless version of the And kernel
vx_status vxAndU8(vx_image in1, vx_image in2, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base[2] = { NULL, NULL };
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(in1, &rect);
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in2, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr[0].dim_x;
    height = src_addr[0].dim_y;
    
    vx_uint32 w16 = width >> 4;
    for (y = 0; y < height; y++) 
    {
        const vx_uint8* src1R = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
        const vx_uint8* src2R = (vx_uint8 *)src_base[1] + y * src_addr[0].stride_y;
        vx_uint8* dstR = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
        for (x = 0; x < w16; x++) 
        {
            uint8x16_t vSrc1R = vld1q_u8(src1R);
            uint8x16_t vSrc2R = vld1q_u8(src2R);
            uint8x16_t vAnd = vandq_u8(vSrc1R, vSrc2R);
            vst1q_u8(dstR, vAnd);

            src2R += 16;
            src1R += 16;
            dstR += 16;
        }
        vx_int16 tmp;
        for (x = (w16 << 4); x < width; x++)
        {
            tmp = (*src1R) & (*src2R);
            *dstR = (vx_uint8)(tmp < 0 ? (-tmp) : tmp);
            src1R++;
            src2R++;
            dstR++;
        }
    }
    status |= vxUnmapImagePatch(in1, map_id_0);
    status |= vxUnmapImagePatch(in2, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);
    return status;
}

// nodeless version of the And kernel
vx_status vxAnd(vx_image in1, vx_image in2, vx_image output)
{
    vx_df_image format;
    vx_status status = vxQueryImage(in1, VX_IMAGE_FORMAT, &format, sizeof(format));
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1)
        return vxBinaryU1Op(in1, in2, output, vx_and_op);
    else
        return vxAndU8(in1, in2, output);
}

// nodeless version of the Or kernel
vx_status vxOrU8(vx_image in1, vx_image in2, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base[2] = { NULL, NULL };
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(in1, &rect);
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in2, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr[0].dim_x;
    height = src_addr[0].dim_y;
    vx_uint32 w16 = width >> 4;
    for (y = 0; y < height; y++) 
    {
        const vx_uint8* src1R = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
        const vx_uint8* src2R = (vx_uint8 *)src_base[1] + y * src_addr[0].stride_y;
        vx_uint8* dstR = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
        for (x = 0; x < w16; x++) 
        {
            uint8x16_t vSrc1R = vld1q_u8(src1R);
            uint8x16_t vSrc2R = vld1q_u8(src2R);
            uint8x16_t vOr = vorrq_u8(vSrc1R, vSrc2R);
            vst1q_u8(dstR, vOr);

            src2R += 16;
            src1R += 16;
            dstR += 16;
        }
        vx_int16 tmp;
        for (x = (w16 << 4); x < width; x++)
        {
            tmp = (*src1R) | (*src2R);
            *dstR = (vx_uint8)(tmp < 0 ? (-tmp) : tmp);
            src1R++;
            src2R++;
            dstR++;
        }
    }
    status |= vxUnmapImagePatch(in1, map_id_0);
    status |= vxUnmapImagePatch(in2, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);
    return status;
}

// nodeless version of the Or kernel
vx_status vxOr(vx_image in1, vx_image in2, vx_image output)
{
    vx_df_image format;
    vx_status status = vxQueryImage(in1, VX_IMAGE_FORMAT, &format, sizeof(format));
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1)
        return vxBinaryU1Op(in1, in2, output, vx_or_op);
    else
        return vxOrU8(in1, in2, output);
}

// nodeless version of the Xor kernel
vx_status vxXorU8(vx_image in1, vx_image in2, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base[2] = { NULL, NULL };
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(in1, &rect);
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in2, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr[0].dim_x;
    height = src_addr[0].dim_y;
    vx_uint32 w16 = width >> 4;
    for (y = 0; y < height; y++)
    {
        const vx_uint8* src1R = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
        const vx_uint8* src2R = (vx_uint8 *)src_base[1] + y * src_addr[0].stride_y;
        vx_uint8* dstR = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
        for (x = 0; x < w16; x++) 
        {
            uint8x16_t vSrc1R = vld1q_u8(src1R);
            uint8x16_t vSrc2R = vld1q_u8(src2R);
            uint8x16_t vXor = veorq_u8(vSrc1R, vSrc2R);
            vst1q_u8(dstR, vXor);

            src2R += 16;
            src1R += 16;
            dstR += 16;
        }
        vx_int16 tmp;
        for (x = (w16 << 4); x < width; x++)
        {
            tmp = (*src1R) ^ (*src2R);
            *dstR = (vx_uint8)(tmp < 0 ? (-tmp) : tmp);
            src1R++;
            src2R++;
            dstR++;
        }
    }
    status |= vxUnmapImagePatch(in1, map_id_0);
    status |= vxUnmapImagePatch(in2, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);
    return status;
}


// nodeless version of the Xor kernel
vx_status vxXor(vx_image in1, vx_image in2, vx_image output)
{
    vx_df_image format;
    vx_status status = vxQueryImage(in1, VX_IMAGE_FORMAT, &format, sizeof(format));
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1)
        return vxBinaryU1Op(in1, in2, output, vx_xor_op);
    else
        return vxXorU8(in1, in2, output);
}

static inline void vxbitwiseNot(const vx_uint8 * input, vx_uint8 * output)
{
    const uint8x16_t val0 = vld1q_u8(input);

    vst1q_u8(output, vmvnq_u8(val0));
}
// nodeless version of the Not kernel
vx_status vxNotU8(vx_image input, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(input, &rect);
    vx_map_id map_id = 0;
    vx_map_id result_map_id = 0;
    status |= vxMapImagePatch(input, &rect, 0, &map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &result_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    height = src_addr.dim_y;
    width = src_addr.dim_x;
    for (y = 0; y < height; y++)
    {
        vx_uint8* srcp = (vx_uint8 *)src_base + y * width;
        vx_uint8* dstp = (vx_uint8 *)dst_base + y * width;

        vx_int32 roiw16 = width >= 15 ? width - 15 : 0;
        x = 0;
        for (; x < roiw16; x+=16)
        {
            vxbitwiseNot(srcp, dstp);
            srcp+=16;
            dstp+=16;
        }
        for (; x < width; x++)
        {
            vx_uint8 *src = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            *dst = ~*src;
        }
    }
    status |= vxUnmapImagePatch(input, map_id);
    status |= vxUnmapImagePatch(output, result_map_id);

    return status;
}


// nodeless version of the Not kernel
vx_status vxNotU1(vx_image input, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    vx_df_image format = 0;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status  = vxGetValidRegionImage(input, &rect);
    status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, (void **)&src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
    width  = (format == VX_DF_IMAGE_U1) ? src_addr.dim_x - rect.start_x % 8 : src_addr.dim_x;
    height = src_addr.dim_y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            vx_uint32 xShftd = (format == VX_DF_IMAGE_U1) ? x + rect.start_x % 8 : x;    // Shift for U1 valid region
            vx_uint8 *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, xShftd, y, &src_addr);
            vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xShftd, y, &dst_addr);

            vx_uint8 mask  = 1 << (xShftd % 8);
            vx_uint8 pixel = ~*src & mask;
            *dst = (*dst & ~mask) | pixel;
        }
    }
    status |= vxCommitImagePatch(input, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);

    return status;
}


vx_status vxNot(vx_image input, vx_image output)
{
    vx_df_image format;
    vx_status status = vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1)
        return vxNotU1(input, output);
    else
        return vxNotU8(input, output);
}
