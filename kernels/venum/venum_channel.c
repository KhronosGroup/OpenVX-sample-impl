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
#include <stdlib.h>

static void procRGB(void *src_ptrs[], vx_imagepatch_addressing_t *src_addrs,
                    void *dst_ptr, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 x, y;
    vx_uint32 wCnt;
    vx_uint8 *ptr0, *ptr1, *ptr2, *pout;
    if (sizeof(vx_uint8) == src_addrs[0].stride_x)
    {
        wCnt = (src_addrs[0].dim_x >> 4) << 4;
        for (y = 0; y < dst_addr->dim_y; y+=dst_addr->step_y)
        {
            ptr0 = (vx_uint8 *)src_ptrs[0] + y * src_addrs[0].stride_y;
            ptr1 = (vx_uint8 *)src_ptrs[1] + y * src_addrs[1].stride_y;
            ptr2 = (vx_uint8 *)src_ptrs[2] + y * src_addrs[2].stride_y;
            pout = (vx_uint8 *)dst_ptr + y * dst_addr->stride_y;
            for (x = 0; x < wCnt; x += 16)
            {
                uint8x16x3_t pixels = {{vld1q_u8(ptr0 + x * src_addrs[0].stride_x),
                    vld1q_u8(ptr1 + x * src_addrs[1].stride_x),
                    vld1q_u8(ptr2 + x * src_addrs[2].stride_x)}};

                vst3q_u8(pout + x * dst_addr->stride_x, pixels);
            }
            for (x = wCnt; x < src_addrs[0].dim_x; x++)
            {
                vx_uint8 *planes[3] = {
                    vxFormatImagePatchAddress2d(src_ptrs[0], x, y, &src_addrs[0]),
                    vxFormatImagePatchAddress2d(src_ptrs[1], x, y, &src_addrs[1]),
                    vxFormatImagePatchAddress2d(src_ptrs[2], x, y, &src_addrs[2]),
                };
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_ptr, x, y, dst_addr);
                dst[0] = planes[0][0];
                dst[1] = planes[1][0];
                dst[2] = planes[2][0];
            }
        }
    }
    return;
}

static void procRGBX(void *src_ptrs[], vx_imagepatch_addressing_t *src_addrs,
                     void *dst_ptr, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 x, y;
    vx_uint32 wCnt;
    vx_uint8 *ptr0, *ptr1, *ptr2, *ptr3, *pout;
    if (sizeof(vx_uint8) == src_addrs[0].stride_x)
    {
        wCnt = (src_addrs[0].dim_x >> 4) << 4;
        for (y = 0; y < dst_addr->dim_y; y+=dst_addr->step_y)
        {
            ptr0 = (vx_uint8 *)src_ptrs[0] + y * src_addrs[0].stride_y;
            ptr1 = (vx_uint8 *)src_ptrs[1] + y * src_addrs[1].stride_y;
            ptr2 = (vx_uint8 *)src_ptrs[2] + y * src_addrs[2].stride_y;
            ptr3 = (vx_uint8 *)src_ptrs[3] + y * src_addrs[3].stride_y;
            pout = (vx_uint8 *)dst_ptr + y * dst_addr->stride_y;
            for (x = 0; x < wCnt; x += 16)
            {
                uint8x16x4_t pixels = {{vld1q_u8(ptr0 + x * src_addrs[0].stride_x),
                    vld1q_u8(ptr1 + x * src_addrs[1].stride_x),
                    vld1q_u8(ptr2 + x * src_addrs[2].stride_x),
                    vld1q_u8(ptr3 + x * src_addrs[3].stride_x)}};

                vst4q_u8(pout + x * dst_addr->stride_x, pixels);
            }
            for (x = wCnt; x < src_addrs[0].dim_x; x++)
            {
                vx_uint8 *planes[4] = {
                    vxFormatImagePatchAddress2d(src_ptrs[0], x, y, &src_addrs[0]),
                    vxFormatImagePatchAddress2d(src_ptrs[1], x, y, &src_addrs[1]),
                    vxFormatImagePatchAddress2d(src_ptrs[2], x, y, &src_addrs[2]),
                    vxFormatImagePatchAddress2d(src_ptrs[3], x, y, &src_addrs[3]),
                };
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_ptr, x, y, dst_addr);
                dst[0] = planes[0][0];
                dst[1] = planes[1][0];
                dst[2] = planes[2][0];
                dst[3] = planes[3][0];
            }
        }
    }
    return;
}

static void procYUV4orIYUV(void *src_ptr, vx_imagepatch_addressing_t *src_addr,
                           void *dst_ptr, vx_imagepatch_addressing_t *dst_addr)
{
    vx_uint32 x, y;
    vx_uint32 wCnt;
    vx_uint8 *ptr_in, *ptr_out;
    if (sizeof(vx_uint8) == dst_addr->stride_x)
    {
        if (1 == dst_addr->step_y)
        {
            wCnt = (dst_addr->dim_x >> 4) << 4;
            for (y = 0; y < dst_addr->dim_y; y += dst_addr->step_y)
            {
                ptr_in = (vx_uint8 *)src_ptr + y * src_addr->stride_y;
                ptr_out = (vx_uint8 *)dst_ptr + y * dst_addr->stride_y;

                for (x = 0; x < wCnt; x += 16)
                {
                    uint8x16_t pixels = vld1q_u8(ptr_in + x * src_addr->stride_x);
                    vst1q_u8(ptr_out + x * dst_addr->stride_x, pixels);
                }

                for (x = wCnt; x < dst_addr->dim_x; x++)
                {
                    vx_uint32 x1 = x * src_addr->step_x / dst_addr->step_x;
                    vx_uint32 y1 = y * src_addr->step_y / dst_addr->step_y;
                    vx_uint8 *src = vxFormatImagePatchAddress2d(src_ptr, x1, y1, src_addr);
                    vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_ptr, x, y, dst_addr);
                    *dst = *src;
                }
            }
        }
        else
        {
            wCnt = ((dst_addr->dim_x >> 1) >> 3) << 3;
            for (y = 0; y < dst_addr->dim_y; y += dst_addr->step_y)
            {
                ptr_in = (vx_uint8 *)src_ptr + ((y * src_addr->step_y / dst_addr->step_y) * src_addr->scale_y / VX_SCALE_UNITY) * src_addr->stride_y;
                ptr_out = (vx_uint8 *)dst_ptr + (y * dst_addr->scale_y / VX_SCALE_UNITY) * dst_addr->stride_y;

                for (x = 0; x < wCnt; x += 8)
                {
                    uint8x8_t pixels = vld1_u8(ptr_in + x * src_addr->stride_x);
                    vst1_u8(ptr_out + x * dst_addr->stride_x, pixels);
                }

                for (x = wCnt << 1; x < dst_addr->dim_x; x+=dst_addr->step_x)
                {
                    vx_uint32 x1 = x * src_addr->step_x / dst_addr->step_x;
                    vx_uint32 y1 = y * src_addr->step_y / dst_addr->step_y;
                    vx_uint8 *src = vxFormatImagePatchAddress2d(src_ptr, x1, y1, src_addr);
                    vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_ptr, x, y, dst_addr);
                    *dst = *src;
                }
            }
        }
    }
    return;
}

// nodeless version of the ChannelCombine kernel
vx_status vxChannelCombine(vx_image inputs[4], vx_image output)
{
    vx_df_image format = 0;
    vx_rectangle_t rect;
    vxQueryImage(output, VX_IMAGE_FORMAT, &format, sizeof(format));
    vxGetValidRegionImage(inputs[0], &rect);
    if ((format == VX_DF_IMAGE_RGB) || (format == VX_DF_IMAGE_RGBX))
    {
        /* write all the channels back out in interleaved format */
        vx_imagepatch_addressing_t src_addrs[4];
        vx_imagepatch_addressing_t dst_addr;
        void *base_src_ptrs[4] = {NULL, NULL, NULL, NULL};
        void *base_dst_ptr = NULL;
        vx_uint32 x, y, p;
        vx_uint32 numplanes = 3;
        vx_uint32 wCnt, wStep;
        vx_uint8 *ptr0, *ptr1, *ptr2, *pout;

        if (format == VX_DF_IMAGE_RGBX)
        {
            numplanes = 4;
        }

        // get the planes
        vx_map_id *map_id_p = calloc(numplanes, sizeof(vx_map_id));
        vx_map_id map_id_dst = 0;
        for (p = 0; p < numplanes; p++)
        {
            vxMapImagePatch(inputs[p], &rect, 0, map_id_p+p, &src_addrs[p], &base_src_ptrs[p], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        }
        vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

        if (format == VX_DF_IMAGE_RGB)
        {
            procRGB(base_src_ptrs, src_addrs, base_dst_ptr, &dst_addr);
        }
        else if(format == VX_DF_IMAGE_RGBX)
        {
            procRGBX(base_src_ptrs, src_addrs, base_dst_ptr, &dst_addr);
        }

        // write the data back
        vxUnmapImagePatch(output, map_id_dst);
        // release the planes
        for (p = 0; p < numplanes; p++)
        {
            vxUnmapImagePatch(inputs[p], *(map_id_p+p));
        }
    }
    else if ((format == VX_DF_IMAGE_YUV4) || (format == VX_DF_IMAGE_IYUV))
    {
        vx_uint32 x, y, p;

        for (p = 0; p < 3; p++)
        {
            vx_imagepatch_addressing_t src_addr;
            vx_imagepatch_addressing_t dst_addr;
            void *base_src_ptr = NULL;
            void *base_dst_ptr = NULL;

            // get the plane
            vx_map_id map_id_p = 0;
            vx_map_id map_id_dst = 0;
            vxMapImagePatch(inputs[p], &rect, 0, &map_id_p, &src_addr, &base_src_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
            vxMapImagePatch(output, &rect, p, &map_id_dst, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

            procYUV4orIYUV(base_src_ptr, &src_addr, base_dst_ptr, &dst_addr);
            
            vxUnmapImagePatch(output, map_id_dst);
            vxUnmapImagePatch(inputs[p], map_id_p);
        }
    }
    else if ((format == VX_DF_IMAGE_NV12) || (format == VX_DF_IMAGE_NV21))
    {
        vx_uint32 x, y;
        int vidx = (format == VX_DF_IMAGE_NV12) ? 1 : 0;

        //plane 0
        {
            vx_imagepatch_addressing_t src_addr;
            vx_imagepatch_addressing_t dst_addr;
            void *base_src_ptr = NULL;
            void *base_dst_ptr = NULL;
            vx_uint32 wCnt;

            // get the plane
            vx_map_id map_id_0 = 0;
            vx_map_id map_id_dst = 0;
            vxMapImagePatch(inputs[0], &rect, 0, &map_id_0, &src_addr, &base_src_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
            vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

            wCnt = (dst_addr.dim_x >> 4) << 4;
            for (y = 0; y < dst_addr.dim_y; y += dst_addr.step_y)
            {
                vx_uint8 *ptr_src = (vx_uint8 *)base_src_ptr + y * src_addr.stride_y;
                vx_uint8 *ptr_dst = (vx_uint8 *)base_dst_ptr + y * dst_addr.stride_y;
                for (x = 0; x < wCnt; x += 16)
                {
                    uint8x16_t pixels = vld1q_u8(ptr_src + x * src_addr.stride_x);
                    vst1q_u8(ptr_dst + x * dst_addr.stride_x, pixels);
                }
                for (x = wCnt; x < dst_addr.dim_x; x += dst_addr.step_x)
                {
                    vx_uint8 *src = vxFormatImagePatchAddress2d(base_src_ptr, x, y, &src_addr);
                    vx_uint8 *dst = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                    *dst = *src;
                }
            }
            vxUnmapImagePatch(output, map_id_dst);
            vxUnmapImagePatch(inputs[0], map_id_0);
        }

        // plane 1
        {
            vx_imagepatch_addressing_t src0_addr;
            vx_imagepatch_addressing_t src1_addr;
            vx_imagepatch_addressing_t dst_addr;
            void *base_src0_ptr = NULL;
            void *base_src1_ptr = NULL;
            void *base_dst_ptr = NULL;
            vx_uint32 wCnt;

            // get the plane
            vx_map_id map_id_1 = 0;
            vx_map_id map_id_2 = 0;
            vx_map_id map_id_dst = 0;
            vxMapImagePatch(inputs[1], &rect, 0, &map_id_1, &src0_addr, &base_src0_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
            vxMapImagePatch(inputs[2], &rect, 0, &map_id_2, &src1_addr, &base_src1_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
            vxMapImagePatch(output, &rect, 1, &map_id_dst, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

            wCnt = ((dst_addr.dim_x >> 1) >> 3) << 3;
            for (y = 0; y < dst_addr.dim_y; y += dst_addr.step_y)
            {
                vx_uint8 *ptr_src0 = (vx_uint8 *)base_src0_ptr + src0_addr.stride_y * ((y * src0_addr.step_y / dst_addr.step_y) * src0_addr.scale_y / VX_SCALE_UNITY);
                vx_uint8 *ptr_src1 = (vx_uint8 *)base_src1_ptr + src1_addr.stride_y * ((y * src1_addr.step_y / dst_addr.step_y) * src1_addr.scale_y / VX_SCALE_UNITY);
                vx_uint8 *ptr_dst = (vx_uint8 *)base_dst_ptr + dst_addr.stride_y * (y * dst_addr.scale_y / VX_SCALE_UNITY);
                for (x = 0; x < wCnt; x += 8)
                {
                    uint8x8x2_t pixels;
                    pixels.val[1-vidx] = vld1_u8(ptr_src0 + x * src0_addr.stride_x);
                    pixels.val[vidx] = vld1_u8(ptr_src1 + x * src1_addr.stride_x);
                    vst2_u8(ptr_dst + x * dst_addr.stride_x, pixels);
                }
                for (x = wCnt << 1; x < dst_addr.dim_x; x += dst_addr.step_x)
                {
                    vx_uint32 x1 = x * src0_addr.step_x / dst_addr.step_x;
                    vx_uint32 y1 = y * src0_addr.step_y / dst_addr.step_y;
                    vx_uint8 *src0 = vxFormatImagePatchAddress2d(base_src0_ptr, x1, y1, &src0_addr);
                    vx_uint8 *src1 = vxFormatImagePatchAddress2d(base_src1_ptr, x1, y1, &src1_addr);
                    vx_uint8 *dst = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                    dst[1-vidx] = *src0;
                    dst[vidx]   = *src1;
                }
            }
            vxUnmapImagePatch(output, map_id_dst);
            vxUnmapImagePatch(inputs[1], map_id_1);
            vxUnmapImagePatch(inputs[2], map_id_2);
        }
    }
    else if ((format == VX_DF_IMAGE_YUYV) || (format == VX_DF_IMAGE_UYVY))
    {
        vx_uint32 x, y;
        int yidx = (format == VX_DF_IMAGE_UYVY) ? 1 : 0;
        vx_imagepatch_addressing_t src0_addr;
        vx_imagepatch_addressing_t src1_addr;
        vx_imagepatch_addressing_t src2_addr;
        vx_imagepatch_addressing_t dst_addr;
        void *base_src0_ptr = NULL;
        void *base_src1_ptr = NULL;
        void *base_src2_ptr = NULL;
        void *base_dst_ptr = NULL;
        vx_uint32 wCnt;

        vx_map_id map_id_0 = 0;
        vx_map_id map_id_1 = 0;
        vx_map_id map_id_2 = 0;
        vx_map_id map_id_dst = 0;
        vxMapImagePatch(inputs[0], &rect, 0, &map_id_0, &src0_addr, &base_src0_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        vxMapImagePatch(inputs[1], &rect, 0, &map_id_1, &src1_addr, &base_src1_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        vxMapImagePatch(inputs[2], &rect, 0, &map_id_2, &src2_addr, &base_src2_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
        vxMapImagePatch(output, &rect, 0, &map_id_dst, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

        wCnt = (dst_addr.dim_x >> 4) << 4;
        for (y = 0; y < dst_addr.dim_y; y += dst_addr.step_y)
        {
            vx_uint8 *ptr_src0 = (vx_uint8 *)base_src0_ptr + src0_addr.stride_y * ((y * src0_addr.step_y / dst_addr.step_y) * src0_addr.scale_y / VX_SCALE_UNITY);
            vx_uint8 *ptr_src1 = (vx_uint8 *)base_src1_ptr + src1_addr.stride_y * ((y * src1_addr.step_y / dst_addr.step_y) * src1_addr.scale_y / VX_SCALE_UNITY);
            vx_uint8 *ptr_src2 = (vx_uint8 *)base_src2_ptr + src2_addr.stride_y * ((y * src2_addr.step_y / dst_addr.step_y) * src2_addr.scale_y / VX_SCALE_UNITY);
            vx_uint8 *ptr_dst = (vx_uint8 *)base_dst_ptr + dst_addr.stride_y * y;
            for (x = 0; x < wCnt; x += 16)
            {
                uint8x8x2_t pixels_y = vld2_u8(ptr_src0 + x * src0_addr.stride_x);
                uint8x8x2_t pixels_uv = {{vld1_u8(ptr_src1 + (x >> 1) * src1_addr.stride_x),
                                          vld1_u8(ptr_src2 + (x >> 1) * src2_addr.stride_x)}};
                uint8x8x4_t pixels;
                pixels.val[0 + yidx] = pixels_y.val[0];
                pixels.val[1 - yidx] = pixels_uv.val[0];
                pixels.val[2 + yidx] = pixels_y.val[1];
                pixels.val[3 - yidx] = pixels_uv.val[1];

                vst4_u8(ptr_dst + x * dst_addr.stride_x, pixels);
            }
            for (x = wCnt; x < dst_addr.dim_x; x += dst_addr.step_x * 2)
            {
                vx_uint32 x1 = x * src0_addr.step_x / dst_addr.step_x;
                vx_uint32 y1 = y * src0_addr.step_y / dst_addr.step_y;
                vx_uint32 x2 = x * src1_addr.step_x / (dst_addr.step_x * 2);
                vx_uint32 y2 = y * src1_addr.step_y / dst_addr.step_y;
                vx_uint8 *srcy0 = vxFormatImagePatchAddress2d(base_src0_ptr, x1, y1, &src0_addr);
                vx_uint8 *srcy1 = vxFormatImagePatchAddress2d(base_src0_ptr, x1 + src0_addr.step_x, y1, &src0_addr);
                vx_uint8 *srcu = vxFormatImagePatchAddress2d(base_src1_ptr, x2, y2, &src1_addr);
                vx_uint8 *srcv = vxFormatImagePatchAddress2d(base_src2_ptr, x2, y2, &src2_addr);
                vx_uint8 *dst0 = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                vx_uint8 *dst1 = vxFormatImagePatchAddress2d(base_dst_ptr, x + dst_addr.step_x, y, &dst_addr);
                dst0[yidx] = *srcy0;
                dst1[yidx] = *srcy1;
                dst0[1-yidx] = *srcu;
                dst1[1-yidx] = *srcv;
            }
        }
        vxUnmapImagePatch(output, map_id_dst);
        vxUnmapImagePatch(inputs[0], map_id_0);
        vxUnmapImagePatch(inputs[1], map_id_1);
        vxUnmapImagePatch(inputs[2], map_id_2);
    }
    return VX_SUCCESS;
}

static vx_status vxCopyPlaneToImage(vx_image src,
                                     vx_uint32 src_plane,
                                     vx_uint8 src_component,
                                     vx_uint32 x_subsampling,
                                     vx_image dst)
{
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = {0};
    vx_imagepatch_addressing_t dst_addr = {0};
    vx_rectangle_t src_rect, dst_rect;
    vx_uint32 x, y;
    vx_status status = VX_SUCCESS;

    status = vxGetValidRegionImage(src, &src_rect);
    vx_map_id map_id_src = 0;
    vx_map_id map_id_dst = 0;
    status |= vxMapImagePatch(src, &src_rect, src_plane, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    if (status == VX_SUCCESS)
    {
        dst_rect = src_rect;
        dst_rect.start_x /= VX_SCALE_UNITY / src_addr.scale_x * x_subsampling;
        dst_rect.start_y /= VX_SCALE_UNITY / src_addr.scale_y;
        dst_rect.end_x /= VX_SCALE_UNITY / src_addr.scale_x * x_subsampling;
        dst_rect.end_y /= VX_SCALE_UNITY / src_addr.scale_y;

        status = vxMapImagePatch(dst, &dst_rect, 0, &map_id_dst, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
        if (status == VX_SUCCESS)
        {
            for (y = 0; y < dst_addr.dim_y; y++)
            {
                vx_uint8* srcyP = (vx_uint8 *)src_base + src_addr.stride_y*((src_addr.scale_y*(y * VX_SCALE_UNITY / src_addr.scale_y))/VX_SCALE_UNITY);
                vx_uint32 roiw16 = dst_addr.dim_x >= 15 ? dst_addr.dim_x - 15 : 0;
                for (x = 0; x < roiw16; x+=16)
                {
                    vx_uint8 *srcp = srcyP + src_addr.stride_x*((src_addr.scale_x *(x* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp1 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+1)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp2 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+2)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp3 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+3)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp4 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+4)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp5 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+5)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp6 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+6)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp7 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+7)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp8 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+8)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp9 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+9)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp10 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+10)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp11 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+11)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp12 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+12)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp13 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+13)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp14 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+14)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    vx_uint8 *srcp15 = srcyP + src_addr.stride_x*((src_addr.scale_x *((x+15)* VX_SCALE_UNITY / src_addr.scale_x * x_subsampling))/VX_SCALE_UNITY);
                    
                    uint8x16_t src8x16;
                    src8x16 = vsetq_lane_u8(srcp[src_component], src8x16, 0);
                    src8x16 = vsetq_lane_u8(srcp1[src_component], src8x16, 1);
                    src8x16 = vsetq_lane_u8(srcp2[src_component], src8x16, 2);
                    src8x16 = vsetq_lane_u8(srcp3[src_component], src8x16, 3);
                    src8x16 = vsetq_lane_u8(srcp4[src_component], src8x16, 4);
                    src8x16 = vsetq_lane_u8(srcp5[src_component], src8x16, 5);
                    src8x16 = vsetq_lane_u8(srcp6[src_component], src8x16, 6);
                    src8x16 = vsetq_lane_u8(srcp7[src_component], src8x16, 7);
                    src8x16 = vsetq_lane_u8(srcp8[src_component], src8x16, 8);
                    src8x16 = vsetq_lane_u8(srcp9[src_component], src8x16, 9);
                    src8x16 = vsetq_lane_u8(srcp10[src_component], src8x16, 10);
                    src8x16 = vsetq_lane_u8(srcp11[src_component], src8x16, 11);
                    src8x16 = vsetq_lane_u8(srcp12[src_component], src8x16, 12);
                    src8x16 = vsetq_lane_u8(srcp13[src_component], src8x16, 13);
                    src8x16 = vsetq_lane_u8(srcp14[src_component], src8x16, 14);
                    src8x16 = vsetq_lane_u8(srcp15[src_component], src8x16, 15);                   
                    vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);                    
                    vst1q_u8 (dstp, src8x16);
                }
                for (; x < dst_addr.dim_x; x++)
                {
                    vx_uint8 *srcp = vxFormatImagePatchAddress2d(src_base,
                        x * VX_SCALE_UNITY / src_addr.scale_x * x_subsampling,
                        y * VX_SCALE_UNITY / src_addr.scale_y,
                        &src_addr);
                    vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                    *dstp = srcp[src_component];
                }
            }
            vxUnmapImagePatch(dst, map_id_dst);
        }
        vxUnmapImagePatch(src, map_id_src);
    }
    return status;
}

// nodeless version of the ChannelExtract kernel
vx_status vxChannelExtract(vx_image src, vx_scalar channel, vx_image dst)
{
    vx_enum chan = -1;
    vx_df_image format = 0;
    vx_uint32 cidx = 0;
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    vxCopyScalar(channel, &chan, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
    switch (format)
    {
        case VX_DF_IMAGE_RGB:
        case VX_DF_IMAGE_RGBX:
            switch (chan)
            {
                case VX_CHANNEL_R:
                    cidx = 0; break;
                case VX_CHANNEL_G:
                    cidx = 1; break;
                case VX_CHANNEL_B:
                    cidx = 2; break;
                case VX_CHANNEL_A:
                    cidx = 3; break;
                default:
                    return VX_ERROR_INVALID_PARAMETERS;
            }
            status = vxCopyPlaneToImage(src, 0, cidx, 1, dst);
            break;
        case VX_DF_IMAGE_NV12:
        case VX_DF_IMAGE_NV21:
            if (chan == VX_CHANNEL_Y)
                status = vxCopyPlaneToImage(src, 0, 0, 1, dst);
            else if ((chan == VX_CHANNEL_U && format == VX_DF_IMAGE_NV12) ||
                     (chan == VX_CHANNEL_V && format == VX_DF_IMAGE_NV21))
                status = vxCopyPlaneToImage(src, 1, 0, 1, dst);
            else
                status = vxCopyPlaneToImage(src, 1, 1, 1, dst);
            break;
        case VX_DF_IMAGE_IYUV:
        case VX_DF_IMAGE_YUV4:
            switch (chan)
            {
                case VX_CHANNEL_Y:
                    cidx = 0; break;
                case VX_CHANNEL_U:
                    cidx = 1; break;
                case VX_CHANNEL_V:
                    cidx = 2; break;
                default:
                    return VX_ERROR_INVALID_PARAMETERS;
            }
            status = vxCopyPlaneToImage(src, cidx, 0, 1, dst);
            break;
        case VX_DF_IMAGE_YUYV:
        case VX_DF_IMAGE_UYVY:
            if (chan == VX_CHANNEL_Y)
                status = vxCopyPlaneToImage(src, 0, format == VX_DF_IMAGE_YUYV ? 0 : 1, 1, dst);
            else
                status = vxCopyPlaneToImage(src, 0,
                    (format == VX_DF_IMAGE_YUYV ? 1 : 0) + (chan == VX_CHANNEL_U ? 0 : 2),
                    2, dst);
            break;
    }
    return status;
}

