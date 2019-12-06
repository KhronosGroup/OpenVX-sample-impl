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

// nodeless version of the XXXX kernel
vx_status vxIntegralImage(vx_image src, vx_image dst)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t rect;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;

    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(src, &rect);

    status |= vxMapImagePatch(src, &rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    uint32x4_t v_zero = vmovq_n_u32(0u);

    // the first iteration
    const vx_uint8 * pixels_ptr = (vx_uint8 *)src_base;
    vx_uint32  *sums = (vx_uint32 *)dst_base;

    uint32x4_t prev = v_zero;
    size_t j = 0u;

    for ( ; j + 7 < src_addr.dim_x; j += 8)
    {
        uint8x8_t el8shr0 = vld1_u8(pixels_ptr + j);
        uint8x8_t el8shr1 = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(el8shr0), 8));
        uint8x8_t el8shr2 = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(el8shr0), 16));
        uint8x8_t el8shr3 = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(el8shr0), 24));

        uint16x8_t el8shr12 =  vaddl_u8(el8shr1, el8shr2);
        uint16x8_t el8shr03 =  vaddl_u8(el8shr0, el8shr3);

        uint16x8_t el8 = vaddq_u16(el8shr12, el8shr03);
        uint16x4_t el4h = vadd_u16(vget_low_u16(el8), vget_high_u16(el8));

        uint32x4_t vsuml = vaddw_u16(prev, vget_low_u16(el8));
        uint32x4_t vsumh = vaddw_u16(prev, el4h);

        vst1q_u32(sums + j, vsuml);
        vst1q_u32(sums + j + 4, vsumh);

        prev = vaddw_u16(prev, vdup_lane_u16(el4h, 3));

    }

    for (vx_uint32 v = vgetq_lane_u32(prev, 3); j < src_addr.dim_x; ++j)
        sums[j] = (v += pixels_ptr[j]);

    // the others
    for (size_t i = 1; i < src_addr.dim_y ; ++i)
    {
        pixels_ptr = (vx_uint8 *)src_base + i * src_addr.stride_y;
        vx_uint32  *prevSum = (vx_uint32 *)dst_base + (i-1) * dst_addr.stride_y / 4; 
        sums = (vx_uint32 *)dst_base + i * dst_addr.stride_y / 4;

        prev = v_zero;
        j = 0u;

        for ( ; j + 7 < src_addr.dim_x; j += 8)
        {
            uint32x4_t vsuml = vld1q_u32(prevSum + j);
            uint32x4_t vsumh = vld1q_u32(prevSum + j + 4);

            uint8x8_t el8shr0 = vld1_u8(pixels_ptr + j);
            uint8x8_t el8shr1 = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(el8shr0), 8));
            uint8x8_t el8shr2 = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(el8shr0), 16));
            uint8x8_t el8shr3 = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(el8shr0), 24));

            vsuml = vaddq_u32(vsuml, prev);
            vsumh = vaddq_u32(vsumh, prev);

            uint16x8_t el8shr12 =  vaddl_u8(el8shr1, el8shr2);
            uint16x8_t el8shr03 =  vaddl_u8(el8shr0, el8shr3);

            uint16x8_t el8 = vaddq_u16(el8shr12, el8shr03);
            uint16x4_t el4h = vadd_u16(vget_low_u16(el8), vget_high_u16(el8));

            vsuml = vaddw_u16(vsuml, vget_low_u16(el8));
            vsumh = vaddw_u16(vsumh, el4h);

            vst1q_u32(sums + j, vsuml);
            vst1q_u32(sums + j + 4, vsumh);

            prev = vaddw_u16(prev, vdup_lane_u16(el4h, 3));

        }

        for (vx_uint32 v = vgetq_lane_u32(prev, 3); j < src_addr.dim_x; ++j)
            sums[j] = (v += pixels_ptr[j]) + prevSum[j];
    }

    status |= vxUnmapImagePatch(src, src_map_id);
    status |= vxUnmapImagePatch(dst, dst_map_id);

    return status;
}

