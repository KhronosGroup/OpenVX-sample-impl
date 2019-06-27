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
#include <stdio.h>

// nodeless version of the And kernel
vx_status vxAnd(vx_image in1, vx_image in2, vx_image output)
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

// nodeless version of the Or kernel
vx_status vxOr(vx_image in1, vx_image in2, vx_image output)
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

// nodeless version of the Xor kernel
vx_status vxXor(vx_image in1, vx_image in2, vx_image output)
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

static inline void vxbitwiseNot(const uint8_t * input, uint8_t * output)
{
    const uint8x16_t val0 = vld1q_u8(input);

    vst1q_u8(output, vmvnq_u8(val0));
}
// nodeless version of the Not kernel
vx_status vxNot(vx_image input, vx_image output)
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

        for (x = 0; x < width; x+=16)
        {
            vxbitwiseNot(srcp, dstp);
            srcp+=16;
            dstp+=16;
        }
    }
    status |= vxUnmapImagePatch(input, map_id);
    status |= vxUnmapImagePatch(output, result_map_id);

    return status;
}