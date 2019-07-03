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

// nodeless version of the Min kernel
vx_status vxMin(vx_image in0, vx_image in1, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base[2] = { NULL, NULL };
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
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in0, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr[0].dim_x;
    height = src_addr[0].dim_y;
    vx_uint32 wWidth;
    switch (out_format)
    {
    case VX_DF_IMAGE_U8:
        wWidth = (width >> 4) << 4;
        for (y = 0; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
            vx_uint8* src1p = (vx_uint8 *)src_base[1] + y * src_addr[1].stride_y;
            vx_uint8* dstp = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (x = 0; x < wWidth; x+=16,src0p+=16,src1p+=16,dstp+=16)
            {

                const uint8x16_t vsrc0 = vld1q_u8( src0p );
                const uint8x16_t vsrc1 = vld1q_u8( src1p );

                vst1q_u8( dstp,vminq_u8( vsrc0, vsrc1 ) );
            }
            for (x = wWidth; x < width; x++)
            {
                vx_uint8 val0 = *(src0p + x * src_addr[0].stride_x);
                vx_uint8 val1 = *(src1p + x * src_addr[1].stride_x);
                *(dstp + x * dst_addr.stride_x) = val0 < val1 ? val0 : val1;
            }
        }

        break;
    case VX_DF_IMAGE_S16:
        wWidth = (width >> 3) << 3;
        for (y = 0; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
            vx_uint8* src1p = (vx_uint8 *)src_base[1] + y * src_addr[1].stride_y;
            vx_uint8* dstp = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (x = 0; x < wWidth; x+=8)
            {
                const int16x8_t vsrc0 = vld1q_s16( (vx_int16 *)src0p );
                const int16x8_t vsrc1 = vld1q_s16( (vx_int16 *)src1p );

                vst1q_s16( (vx_int16 *)dstp, vminq_s16( vsrc0, vsrc1 ) );
                src0p += 8*src_addr[0].stride_x;
                src1p += 8*src_addr[1].stride_x;
                dstp += 8*dst_addr.stride_x;
            }
            for (x = wWidth; x < width; x++)
            {
                vx_int16 val0 = *(vx_int16 *)(src0p + x * src_addr[0].stride_x);
                vx_int16 val1 = *(vx_int16 *)(src1p + x * src_addr[1].stride_x);
                *(vx_int16 *)(dstp + x * dst_addr.stride_x) = val0 < val1 ? val0 : val1;
            }
        }
        break;

    }
    status |= vxUnmapImagePatch(in0, map_id_0);
    status |= vxUnmapImagePatch(in1, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);

    return status;
}

vx_status vxMax(vx_image in0, vx_image in1, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base[2] = { NULL, NULL };
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_df_image in0_format = 0;
    vx_df_image in1_format = 0;
    vx_df_image out_format = 0;
    vx_status status = VX_SUCCESS;
    vx_uint32 wWidth;

    vxQueryImage(output, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(in0, VX_IMAGE_FORMAT, &in0_format, sizeof(in0_format));
    vxQueryImage(in1, VX_IMAGE_FORMAT, &in1_format, sizeof(in1_format));

    status = vxGetValidRegionImage(in0, &rect);
    vx_map_id map_id_0 = 0;
    vx_map_id map_id_1 = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(in0, &rect, 0, &map_id_0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(in1, &rect, 0, &map_id_1, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr[0].dim_x;
    height = src_addr[0].dim_y;
    switch (out_format)
    {
    case VX_DF_IMAGE_U8:
        wWidth = (width >> 4) << 4;
        for (y = 0; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
            vx_uint8* src1p = (vx_uint8 *)src_base[1] + y * src_addr[1].stride_y;
            vx_uint8* dstp = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (x = 0; x < wWidth; x += 16)
            {
                uint8x16_t vsrc0 = vld1q_u8( src0p + x);
                uint8x16_t vsrc1 = vld1q_u8( src1p + x);

                vst1q_u8( dstp + x, vmaxq_u8( vsrc0, vsrc1 ) );
            }
            for (x = wWidth; x < width; x++)
            {
                vx_uint8 val0 = *(src0p + x);
                vx_uint8 val1 = *(src1p + x);
                *(dstp + x) = val0 > val1 ? val0 : val1;
            }
        }

        break;
    case VX_DF_IMAGE_S16:
        wWidth = (width >> 3) << 3;
        for (y = 0; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)src_base[0] + y * src_addr[0].stride_y;
            vx_uint8* src1p = (vx_uint8 *)src_base[1] + y * src_addr[1].stride_y;
            vx_uint8* dstp = (vx_uint8 *)dst_base + y * dst_addr.stride_y;
            for (x = 0; x < wWidth; x += 8)
            {
                int16x8_t vsrc0 = vld1q_s16( (vx_int16 *)(src0p + x * src_addr[0].stride_x));
                int16x8_t vsrc1 = vld1q_s16( (vx_int16 *)(src1p + x * src_addr[1].stride_x));

                vst1q_s16( (vx_int16 *)(dstp + x * dst_addr.stride_x), vmaxq_s16( vsrc0, vsrc1 ) );
            }
            for (x = wWidth; x < width; x++)
            {
                vx_int16 val0 = *(vx_int16 *)(src0p + x * src_addr[0].stride_x);
                vx_int16 val1 = *(vx_int16 *)(src1p + x * src_addr[1].stride_x);
                *(vx_int16 *)(dstp + x * dst_addr.stride_x) = val0 > val1 ? val0 : val1;
            }
        }
        break;

    }
    status |= vxUnmapImagePatch(in0, map_id_0);
    status |= vxUnmapImagePatch(in1, map_id_1);
    status |= vxUnmapImagePatch(output, map_id_dst);
    return status;
}
