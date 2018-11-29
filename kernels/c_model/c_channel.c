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
        uint32_t x, y, p;
        uint32_t numplanes = 3;

        if (format == VX_DF_IMAGE_RGBX)
        {
            numplanes = 4;
        }

        // get the planes
        for (p = 0; p < numplanes; p++)
        {
            vxAccessImagePatch(inputs[p], &rect, 0, &src_addrs[p], &base_src_ptrs[p], VX_READ_ONLY);
        }
        vxAccessImagePatch(output, &rect, 0, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY);
        for (y = 0; y < dst_addr.dim_y; y+=dst_addr.step_y)
        {
            for (x = 0; x < dst_addr.dim_x; x+=dst_addr.step_x)
            {
                uint8_t *planes[4] = {
                    vxFormatImagePatchAddress2d(base_src_ptrs[0], x, y, &src_addrs[0]),
                    vxFormatImagePatchAddress2d(base_src_ptrs[1], x, y, &src_addrs[1]),
                    vxFormatImagePatchAddress2d(base_src_ptrs[2], x, y, &src_addrs[2]),
                    NULL,
                };
                uint8_t *dst = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                dst[0] = planes[0][0];
                dst[1] = planes[1][0];
                dst[2] = planes[2][0];
                if (format == VX_DF_IMAGE_RGBX)
                {
                    planes[3] = vxFormatImagePatchAddress2d(base_src_ptrs[3], x, y, &src_addrs[3]);
                    dst[3] = planes[3][0];
                }
            }
        }
        // write the data back
        vxCommitImagePatch(output, &rect, 0, &dst_addr, base_dst_ptr);
        // release the planes
        for (p = 0; p < numplanes; p++)
        {
            vxCommitImagePatch(inputs[p], 0, 0, &src_addrs[p], &base_src_ptrs[p]);
        }
    }
    else if ((format == VX_DF_IMAGE_YUV4) || (format == VX_DF_IMAGE_IYUV))
    {
        uint32_t x, y, p;

        for (p = 0; p < 3; p++)
        {
            vx_imagepatch_addressing_t src_addr;
            vx_imagepatch_addressing_t dst_addr;
            void *base_src_ptr = NULL;
            void *base_dst_ptr = NULL;

            // get the plane
            vxAccessImagePatch(inputs[p], &rect, 0, &src_addr, &base_src_ptr, VX_READ_ONLY);
            vxAccessImagePatch(output, &rect, p, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY);

            for (y = 0; y < dst_addr.dim_y; y += dst_addr.step_y)
            {
                for (x = 0; x < dst_addr.dim_x; x += dst_addr.step_x)
                {
                    uint32_t x1 = x * src_addr.step_x / dst_addr.step_x;
                    uint32_t y1 = y * src_addr.step_y / dst_addr.step_y;
                    uint8_t *src = vxFormatImagePatchAddress2d(base_src_ptr, x1, y1, &src_addr);
                    uint8_t *dst = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                    *dst = *src;
                }
            }

            // write the data back
            vxCommitImagePatch(output, &rect, p, &dst_addr, base_dst_ptr);
            // release the input
            vxCommitImagePatch(inputs[p], 0, 0, &src_addr, &base_src_ptr);
        }
    }
    else if ((format == VX_DF_IMAGE_NV12) || (format == VX_DF_IMAGE_NV21))
    {
        uint32_t x, y;
        int vidx = (format == VX_DF_IMAGE_NV12) ? 1 : 0;

        //plane 0
        {
            vx_imagepatch_addressing_t src_addr;
            vx_imagepatch_addressing_t dst_addr;
            void *base_src_ptr = NULL;
            void *base_dst_ptr = NULL;

            // get the plane
            vxAccessImagePatch(inputs[0], &rect, 0, &src_addr, &base_src_ptr, VX_READ_ONLY);
            vxAccessImagePatch(output, &rect, 0, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY);

            for (y = 0; y < dst_addr.dim_y; y += dst_addr.step_y)
            {
                for (x = 0; x < dst_addr.dim_x; x += dst_addr.step_x)
                {
                    uint8_t *src = vxFormatImagePatchAddress2d(base_src_ptr, x, y, &src_addr);
                    uint8_t *dst = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                    *dst = *src;
                }
            }

            // write the data back
            vxCommitImagePatch(output, &rect, 0, &dst_addr, base_dst_ptr);
            // release the input
            vxCommitImagePatch(inputs[0], 0, 0, &src_addr, &base_src_ptr);
        }

        // plane 1
        {
            vx_imagepatch_addressing_t src0_addr;
            vx_imagepatch_addressing_t src1_addr;
            vx_imagepatch_addressing_t dst_addr;
            void *base_src0_ptr = NULL;
            void *base_src1_ptr = NULL;
            void *base_dst_ptr = NULL;

            // get the plane
            vxAccessImagePatch(inputs[1], &rect, 0, &src0_addr, &base_src0_ptr, VX_READ_ONLY);
            vxAccessImagePatch(inputs[2], &rect, 0, &src1_addr, &base_src1_ptr, VX_READ_ONLY);
            vxAccessImagePatch(output, &rect, 1, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY);

            for (y = 0; y < dst_addr.dim_y; y += dst_addr.step_y)
            {
                for (x = 0; x < dst_addr.dim_x; x += dst_addr.step_x)
                {
                    uint32_t x1 = x * src0_addr.step_x / dst_addr.step_x;
                    uint32_t y1 = y * src0_addr.step_y / dst_addr.step_y;
                    uint8_t *src0 = vxFormatImagePatchAddress2d(base_src0_ptr, x1, y1, &src0_addr);
                    uint8_t *src1 = vxFormatImagePatchAddress2d(base_src1_ptr, x1, y1, &src1_addr);
                    uint8_t *dst = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                    dst[1-vidx] = *src0;
                    dst[vidx]   = *src1;
                }
            }

            // write the data back
            vxCommitImagePatch(output, &rect, 1, &dst_addr, base_dst_ptr);
            // release the input
            vxCommitImagePatch(inputs[1], 0, 0, &src0_addr, &base_src0_ptr);
            vxCommitImagePatch(inputs[2], 0, 0, &src1_addr, &base_src1_ptr);
        }
    }
    else if ((format == VX_DF_IMAGE_YUYV) || (format == VX_DF_IMAGE_UYVY))
    {
        uint32_t x, y;
        int yidx = (format == VX_DF_IMAGE_UYVY) ? 1 : 0;
        vx_imagepatch_addressing_t src0_addr;
        vx_imagepatch_addressing_t src1_addr;
        vx_imagepatch_addressing_t src2_addr;
        vx_imagepatch_addressing_t dst_addr;
        void *base_src0_ptr = NULL;
        void *base_src1_ptr = NULL;
        void *base_src2_ptr = NULL;
        void *base_dst_ptr = NULL;

        vxAccessImagePatch(inputs[0], &rect, 0, &src0_addr, &base_src0_ptr, VX_READ_ONLY);
        vxAccessImagePatch(inputs[1], &rect, 0, &src1_addr, &base_src1_ptr, VX_READ_ONLY);
        vxAccessImagePatch(inputs[2], &rect, 0, &src2_addr, &base_src2_ptr, VX_READ_ONLY);
        vxAccessImagePatch(output, &rect, 0, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY);

        for (y = 0; y < dst_addr.dim_y; y += dst_addr.step_y)
        {
            for (x = 0; x < dst_addr.dim_x; x += dst_addr.step_x * 2)
            {
                uint32_t x1 = x * src0_addr.step_x / dst_addr.step_x;
                uint32_t y1 = y * src0_addr.step_y / dst_addr.step_y;
                uint32_t x2 = x * src1_addr.step_x / (dst_addr.step_x * 2);
                uint32_t y2 = y * src1_addr.step_y / dst_addr.step_y;
                uint8_t *srcy0 = vxFormatImagePatchAddress2d(base_src0_ptr, x1, y1, &src0_addr);
                uint8_t *srcy1 = vxFormatImagePatchAddress2d(base_src0_ptr, x1 + src0_addr.step_x, y1, &src0_addr);
                uint8_t *srcu = vxFormatImagePatchAddress2d(base_src1_ptr, x2, y2, &src1_addr);
                uint8_t *srcv = vxFormatImagePatchAddress2d(base_src2_ptr, x2, y2, &src2_addr);
                uint8_t *dst0 = vxFormatImagePatchAddress2d(base_dst_ptr, x, y, &dst_addr);
                uint8_t *dst1 = vxFormatImagePatchAddress2d(base_dst_ptr, x + dst_addr.step_x, y, &dst_addr);
                dst0[yidx] = *srcy0;
                dst1[yidx] = *srcy1;
                dst0[1-yidx] = *srcu;
                dst1[1-yidx] = *srcv;
            }
        }

        vxCommitImagePatch(output, &rect, 0, &dst_addr, base_dst_ptr);
        vxCommitImagePatch(inputs[0], 0, 0, &src0_addr, &base_src0_ptr);
        vxCommitImagePatch(inputs[1], 0, 0, &src1_addr, &base_src1_ptr);
        vxCommitImagePatch(inputs[2], 0, 0, &src2_addr, &base_src2_ptr);
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
    status |= vxAccessImagePatch(src, &src_rect, src_plane, &src_addr, &src_base, VX_READ_ONLY);
    if (status == VX_SUCCESS)
    {
        dst_rect = src_rect;
        dst_rect.start_x /= VX_SCALE_UNITY / src_addr.scale_x * x_subsampling;
        dst_rect.start_y /= VX_SCALE_UNITY / src_addr.scale_y;
        dst_rect.end_x /= VX_SCALE_UNITY / src_addr.scale_x * x_subsampling;
        dst_rect.end_y /= VX_SCALE_UNITY / src_addr.scale_y;

        status = vxAccessImagePatch(dst, &dst_rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
        if (status == VX_SUCCESS)
        {
            for (y = 0; y < dst_addr.dim_y; y++)
            {
                for (x = 0; x < dst_addr.dim_x; x++)
                {
                    vx_uint8 *srcp = vxFormatImagePatchAddress2d(src_base,
                        x * VX_SCALE_UNITY / src_addr.scale_x * x_subsampling,
                        y * VX_SCALE_UNITY / src_addr.scale_y,
                        &src_addr);
                    vx_uint8 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                    *dstp = srcp[src_component];
                }
            }
            vxCommitImagePatch(dst, &dst_rect, 0, &dst_addr, dst_base);
        }
        vxCommitImagePatch(src, NULL, src_plane, &src_addr, src_base);
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

