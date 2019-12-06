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

#include <arm_neon.h>
#include <tiling.h>

void ChannelCombine_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0, p;
    vx_tile_ex_t *in[4];
    in[0] = (vx_tile_ex_t *)parameters[0];
    in[1] = (vx_tile_ex_t *)parameters[1];
    in[2] = (vx_tile_ex_t *)parameters[2];
    in[3] = (vx_tile_ex_t *)parameters[3];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[4];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = out->tile_x + out->tile_block.width;

    void *base_src_ptrs[4] = { NULL };
    void *base_dst_ptr[4] = { NULL };

    base_src_ptrs[0] = in[0]->base[0];
    base_src_ptrs[1] = in[1]->base[0];
    base_src_ptrs[2] = in[2]->base[0];
    base_src_ptrs[3] = in[3]->base[0];

    base_dst_ptr[0] = out->base[0];
    base_dst_ptr[1] = out->base[1];
    base_dst_ptr[2] = out->base[2];
    base_dst_ptr[3] = out->base[3];

    vx_df_image format;

    format = out->image.format;

    vx_uint8 *planes[4];

    if (format == VX_DF_IMAGE_RGB)
    {
        vx_uint8 *ptr0, *ptr1, *ptr2, *pout;
        for (y = low_y; y < high_y; y += out->addr[0].step_y)
        {
            ptr0 = (vx_uint8 *)base_src_ptrs[0] + y * in[0]->addr->stride_y;
            ptr1 = (vx_uint8 *)base_src_ptrs[1] + y * in[1]->addr->stride_y;
            ptr2 = (vx_uint8 *)base_src_ptrs[2] + y * in[2]->addr->stride_y;
            pout = (vx_uint8 *)base_dst_ptr[0] + y * out->addr->stride_y;
            for (x = low_x; x < high_x; x += 16)
            {
                uint8x16x3_t pixels = {{vld1q_u8(ptr0 + x * in[0]->addr->stride_x),
                                        vld1q_u8(ptr1 + x * in[1]->addr->stride_x),
                                        vld1q_u8(ptr2 + x * in[2]->addr->stride_x)}};

                vst3q_u8(pout + x * out->addr->stride_x, pixels);
            }
        }
    }
    else if (format == VX_DF_IMAGE_RGBX)
    {
        vx_uint8 *ptr0, *ptr1, *ptr2, *ptr3, *pout;
        for (y = low_y; y < high_y; y += out->addr[0].step_y)
        {
            ptr0 = (vx_uint8 *)base_src_ptrs[0] + y * in[0]->addr->stride_y;
            ptr1 = (vx_uint8 *)base_src_ptrs[1] + y * in[1]->addr->stride_y;
            ptr2 = (vx_uint8 *)base_src_ptrs[2] + y * in[2]->addr->stride_y;
            ptr3 = (vx_uint8 *)base_src_ptrs[3] + y * in[3]->addr->stride_y;
            pout = (vx_uint8 *)base_dst_ptr[0] + y * out->addr->stride_y;
            for (x = low_x; x < high_x; x += 16)
            {
                uint8x16x4_t pixels = {{vld1q_u8(ptr0 + x * in[0]->addr->stride_x),
                                        vld1q_u8(ptr1 + x * in[1]->addr->stride_x),
                                        vld1q_u8(ptr2 + x * in[2]->addr->stride_x),
                                        vld1q_u8(ptr3 + x * in[3]->addr->stride_x)}};

                vst4q_u8(pout + x * out->addr->stride_x, pixels);
            }
        }
    }
    else if ((format == VX_DF_IMAGE_YUV4) || (format == VX_DF_IMAGE_IYUV))
    {
        vx_uint8 *ptr_in, *ptr_out;
        vx_uint32 wCnt = ((high_x >> 1) >> 3) << 3;
        for (p = 0; p < 3; p++)
        {
            if (1 == out->addr[p].step_y)
            {
                for (y = low_y; y < high_y; y += out->addr[p].step_y)
                {
                    ptr_in = (vx_uint8 *)base_src_ptrs[p] + y * in[p]->addr->stride_y;
                    ptr_out = (vx_uint8 *)base_dst_ptr[p] + y * out->addr[p].stride_y;

                    for (x = low_x; x < high_x; x += 16)
                    {
                        uint8x16_t pixels = vld1q_u8(ptr_in + x * in[p]->addr->stride_x);
                        vst1q_u8(ptr_out + x * out->addr[p].stride_x, pixels);
                    }
                }
            }
            else
            {
                for (y = low_y; y < high_y; y += out->addr[p].step_y)
                {
                    ptr_in = (vx_uint8 *)base_src_ptrs[p] + ((y * in[p]->addr->step_y / out->addr[p].step_y) * 
                             in[p]->addr->scale_y / VX_SCALE_UNITY) * in[p]->addr->stride_y;
                    ptr_out = (vx_uint8 *)base_dst_ptr[p] + (y * out->addr[p].scale_y / VX_SCALE_UNITY) * out->addr[p].stride_y;

                    for (x = low_x; x < wCnt; x += 8)
                    {
                        uint8x8_t pixels = vld1_u8(ptr_in + x * in[p]->addr->stride_x);
                        vst1_u8(ptr_out + x * out->addr[p].stride_x, pixels);
                    }
                }
            }
        }
    }
    else if ((format == VX_DF_IMAGE_NV12) || (format == VX_DF_IMAGE_NV21))
    {
        int vidx = (format == VX_DF_IMAGE_NV12) ? 1 : 0;

        //plane 0
        {
            for (y = low_y; y < high_y; y += out->addr[0].step_y)
            {
                vx_uint8 *ptr_src = (vx_uint8 *)base_src_ptrs[0] + y * in[0]->addr->stride_y;
                vx_uint8 *ptr_dst = (vx_uint8 *)base_dst_ptr[0] + y * out->addr[0].stride_y;
                for (x = low_x; x < high_x; x += 16)
                {
                    uint8x16_t pixels = vld1q_u8(ptr_src + x * in[0]->addr->stride_x);
                    vst1q_u8(ptr_dst + x * out->addr[0].stride_x, pixels);
                }
            }
        }

        // plane 1
        {
            vx_uint32 wCnt = ((high_x >> 1) >> 3) << 3;
            for (y = low_y; y < high_y; y += out->addr[1].step_y)
            {
                vx_uint8 *ptr_src0 = (vx_uint8 *)base_src_ptrs[1] + in[1]->addr->stride_y * 
                                     ((y * in[1]->addr->step_y / out->addr[1].step_y) * in[1]->addr->scale_y / VX_SCALE_UNITY);
                vx_uint8 *ptr_src1 = (vx_uint8 *)base_src_ptrs[2] + in[2]->addr->stride_y * 
                                     ((y * in[1]->addr->step_y / out->addr[1].step_y) * in[2]->addr->scale_y / VX_SCALE_UNITY);
                vx_uint8 *ptr_dst = (vx_uint8 *)base_dst_ptr[1] + out->addr[1].stride_y * (y *out->addr[1].scale_y / VX_SCALE_UNITY);
                for (x = low_x; x < wCnt; x += 8)
                {
                    uint8x8x2_t pixels;
                    pixels.val[1-vidx] = vld1_u8(ptr_src0 + x * in[1]->addr->stride_x);
                    pixels.val[vidx] = vld1_u8(ptr_src1 + x * in[2]->addr->stride_x);
                    vst2_u8(ptr_dst + x * out->addr[1].stride_x, pixels);
                }
            }
        }
    }
    else if ((format == VX_DF_IMAGE_YUYV) || (format == VX_DF_IMAGE_UYVY))
    {
        int yidx = (format == VX_DF_IMAGE_UYVY) ? 1 : 0;
        for (y = low_y; y < high_y; y += out->addr[0].step_y)
        {
            vx_uint8 *ptr_src0 = (vx_uint8 *)base_src_ptrs[0] + in[0]->addr->stride_y * 
                                 ((y * in[0]->addr->step_y / out->addr->step_y) * in[0]->addr->scale_y / VX_SCALE_UNITY);
            vx_uint8 *ptr_src1 = (vx_uint8 *)base_src_ptrs[1] + in[1]->addr->stride_y * 
                                 ((y * in[1]->addr->step_y / out->addr->step_y) * in[1]->addr->scale_y / VX_SCALE_UNITY);
            vx_uint8 *ptr_src2 = (vx_uint8 *)base_src_ptrs[2] + in[2]->addr->stride_y * 
                                 ((y * in[1]->addr->step_y / out->addr->step_y) * in[2]->addr->scale_y / VX_SCALE_UNITY);
            vx_uint8 *ptr_dst = (vx_uint8 *)base_dst_ptr[0] + out->addr[0].stride_y * y;
            for (x = low_x; x < high_x; x += 16)
            {
                uint8x8x2_t pixels_y = vld2_u8(ptr_src0 + x * in[0]->addr->stride_x);
                uint8x8x2_t pixels_uv = {{vld1_u8(ptr_src1 + (x >> 1) * in[1]->addr->stride_x),
                                          vld1_u8(ptr_src2 + (x >> 1) * in[2]->addr->stride_x)}};
                uint8x8x4_t pixels;
                pixels.val[0 + yidx] = pixels_y.val[0];
                pixels.val[1 - yidx] = pixels_uv.val[0];
                pixels.val[2 + yidx] = pixels_y.val[1];
                pixels.val[3 - yidx] = pixels_uv.val[1];

                vst4_u8(ptr_dst + x * out->addr[0].stride_x, pixels);
            }
        }
    }
}

#define RGB(low_y, high_y, low_x)                                                                                   \
    for (y = low_y; y < high_y; y += out->addr->step_y)                                                             \
    {                                                                                                               \
        planes[0] = (vx_uint8 *)base_src_ptrs[0] + y * in[0]->addr->stride_y;                                       \
        planes[1] = (vx_uint8 *)base_src_ptrs[1] + y * in[1]->addr->stride_y;                                       \
        planes[2] = (vx_uint8 *)base_src_ptrs[2] + y * in[2]->addr->stride_y;                                       \
        vx_uint8 *dst = (vx_uint8 *)base_dst_ptr[0] + y * out->addr->stride_y;                                      \
        for (x = low_x; x < high_x; x += out->addr->step_x)                                                         \
        {                                                                                                           \
            dst[0] = planes[0][0];                                                                                  \
            dst[1] = planes[1][0];                                                                                  \
            dst[2] = planes[2][0];                                                                                  \
            if (format == VX_DF_IMAGE_RGBX)                                                                         \
            {                                                                                                       \
                planes[3] = (vx_uint8 *)base_src_ptrs[3] + y * in[3]->addr->stride_y + x * in[3]->addr->stride_x;   \
                dst[3] = planes[3][0];                                                                              \
            }                                                                                                       \
            planes[0] += out->addr->step_x * in[0]->addr->stride_x;                                                 \
            planes[1] += out->addr->step_x * in[1]->addr->stride_x;                                                 \
            planes[2] += out->addr->step_x * in[2]->addr->stride_x;                                                 \
            dst += out->addr->step_x * out->addr->stride_x;                                                         \
        }                                                                                                           \
    }


#define YUV4(low_y, high_y, low_x)                                                                                                     \
    for (p = 0; p < 3; p++)                                                                                                            \
    {                                                                                                                                  \
        for (y = low_y; y < high_y; y += out->addr[p].step_y)                                                                          \
        {                                                                                                                              \
            for (x = low_x; x < high_x; x += out->addr[p].step_x)                                                                      \
            {                                                                                                                          \
                vx_uint32 x1 = x * in[p]->addr->step_x / out->addr[p].step_x;                                                          \
                vx_uint32 y1 = y * in[p]->addr->step_y / out->addr[p].step_y;                                                          \
                vx_uint8 *src = (vx_uint8 *)base_src_ptrs[p] + y1 * in[p]->addr->stride_y + x1 * in[p]->addr->stride_x;                \
                vx_uint8 *dst = (vx_uint8 *)base_dst_ptr[p] + out->addr[p].stride_y * (out->addr[p].scale_y * y) / VX_SCALE_UNITY +    \
                                out->addr[p].stride_x * (out->addr[p].scale_x * x) / VX_SCALE_UNITY;                                   \
                *dst = *src;                                                                                                           \
            }                                                                                                                          \
        }                                                                                                                              \
    }


#define NV12(low_y, high_y, low_x)                                                                                                 \
    for (y = low_y; y < high_y; y += out->addr[0].step_y)                                                                          \
    {                                                                                                                              \
        vx_uint8 *src = (vx_uint8 *)base_src_ptrs[0] + y * in[0]->addr->stride_y;                                                  \
        vx_uint8 *dst = (vx_uint8 *)base_dst_ptr[0] + y * out->addr[0].stride_y;                                                   \
        for (x = low_x; x < high_x; x += out->addr[0].step_x)                                                                      \
        {                                                                                                                          \
            *dst = *src;                                                                                                           \
                                                                                                                                   \
            src += out->addr[0].step_x * in[0]->addr->stride_x;                                                                    \
            dst += out->addr[0].step_x * out->addr[0].stride_x;                                                                    \
        }                                                                                                                          \
    }                                                                                                                              \
                                                                                                                                   \
    for (y = low_y; y < high_y; y += out->addr[1].step_y)                                                                          \
    {                                                                                                                              \
        for (x = low_x; x < high_x; x += out->addr[1].step_x)                                                                      \
        {                                                                                                                          \
            vx_uint32 x1 = x * in[1]->addr->step_x / out->addr[1].step_x;                                                          \
            vx_uint32 y1 = y * in[1]->addr->step_y / out->addr[1].step_y;                                                          \
            vx_uint8 *src0 = (vx_uint8 *)base_src_ptrs[1] + y1 * in[1]->addr->stride_y + x1 * in[1]->addr->stride_x;               \
            vx_uint8 *src1 = (vx_uint8 *)base_src_ptrs[2] + y1 * in[2]->addr->stride_y + x1 * in[2]->addr->stride_x;               \
            vx_uint8 *dst = (vx_uint8 *)base_dst_ptr[1] + out->addr[1].stride_y * (out->addr[1].scale_y * y) / VX_SCALE_UNITY +    \
                            out->addr[1].stride_x * (out->addr[1].scale_x * x) / VX_SCALE_UNITY;                                   \
            dst[1 - vidx] = *src0;                                                                                                 \
            dst[vidx] = *src1;                                                                                                     \
        }                                                                                                                          \
    }                   


#define YUYV(low_y, high_y, low_x)                                                                                                  \
    for (y = low_y; y < high_y; y += out->addr->step_y)                                                                             \
    {                                                                                                                               \
        for (x = low_x; x < high_x; x += out->addr->step_x * 2)                                                                     \
        {                                                                                                                           \
            vx_uint32 x1 = x * in[0]->addr->step_x / out->addr->step_x;                                                             \
            vx_uint32 y1 = y * in[0]->addr->step_y / out->addr->step_y;                                                             \
            vx_uint32 x2 = x * in[1]->addr->step_x / (out->addr->step_x * 2);                                                       \
            vx_uint32 y2 = y * in[1]->addr->step_y / out->addr->step_y;                                                             \
            vx_uint8 *srcy0 = (vx_uint8 *)base_src_ptrs[0] + y1 * in[0]->addr->stride_y + x1 * in[0]->addr->stride_x;               \
            vx_uint8 *srcy1 = (vx_uint8 *)base_src_ptrs[0] + y1 * in[0]->addr->stride_y +                                           \
                              (x1 + in[0]->addr->step_x) * in[0]->addr->stride_x;                                                   \
            vx_uint8 *srcu = (vx_uint8 *)base_src_ptrs[1] + y2 * in[1]->addr->stride_y + x2 * in[1]->addr->stride_x;                \
            vx_uint8 *srcv = (vx_uint8 *)base_src_ptrs[2] + y2 * in[2]->addr->stride_y + x2 * in[2]->addr->stride_x;                \
            vx_uint8 *dst0 = (vx_uint8 *)base_dst_ptr[0] + out->addr[0].stride_y * (out->addr[0].scale_y * y) / VX_SCALE_UNITY +    \
                             out->addr[0].stride_x * (out->addr[0].scale_x * x) / VX_SCALE_UNITY;                                   \
            vx_uint8 *dst1 = (vx_uint8 *)base_dst_ptr[0] + out->addr[0].stride_y * (out->addr[0].scale_y * y) / VX_SCALE_UNITY +    \
                             out->addr[0].stride_x * (out->addr[0].scale_x * (x + out->addr[0].step_x)) / VX_SCALE_UNITY;           \
                                                                                                                                    \
            dst0[yidx] = *srcy0;                                                                                                    \
            dst1[yidx] = *srcy1;                                                                                                    \
            dst0[1 - yidx] = *srcu;                                                                                                 \
            dst1[1 - yidx] = *srcv;                                                                                                 \
        }                                                                                                                           \
    }


void ChannelCombine_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0, p;
    vx_tile_ex_t *in[4];
    in[0] = (vx_tile_ex_t *)parameters[0];
    in[1] = (vx_tile_ex_t *)parameters[1];
    in[2] = (vx_tile_ex_t *)parameters[2];
    in[3] = (vx_tile_ex_t *)parameters[3];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[4];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    void *base_src_ptrs[4] = { NULL };
    void *base_dst_ptr[4] = { NULL };

    base_src_ptrs[0] = in[0]->base[0];
    base_src_ptrs[1] = in[1]->base[0];
    base_src_ptrs[2] = in[2]->base[0];
    base_src_ptrs[3] = in[3]->base[0];

    base_dst_ptr[0] = out->base[0];
    base_dst_ptr[1] = out->base[1];
    base_dst_ptr[2] = out->base[2];
    base_dst_ptr[3] = out->base[3];

    vx_df_image format;

    format = out->image.format;

    vx_uint8 *planes[4];

    if ((format == VX_DF_IMAGE_RGB) || (format == VX_DF_IMAGE_RGBX))
    {
        if (low_y == 0 && low_x == 0)
        {
            RGB(low_y, high_y, low_x)
        }
        else
        {
            RGB(0, low_y, low_x)
            RGB(low_y, high_y, 0)
        }
    }
    else if ((format == VX_DF_IMAGE_YUV4) || (format == VX_DF_IMAGE_IYUV))
    {
        if (low_y == 0 && low_x == 0)
        {
            YUV4(low_y, high_y, low_x)
        }
        else
        {
            YUV4(0, low_y, low_x)
            YUV4(low_y, high_y, 0)
        }
    }
    else if ((format == VX_DF_IMAGE_NV12) || (format == VX_DF_IMAGE_NV21))
    {
        int vidx = (format == VX_DF_IMAGE_NV12) ? 1 : 0;
        if (low_y == 0 && low_x == 0)
        {
            NV12(low_y, high_y, low_x)
        }
        else
        {
            NV12(0, low_y, low_x)
            NV12(low_y, high_y, 0)
        }
    }
    else if ((format == VX_DF_IMAGE_YUYV) || (format == VX_DF_IMAGE_UYVY))
    {
        int yidx = (format == VX_DF_IMAGE_UYVY) ? 1 : 0;

        if (low_y == 0 && low_x == 0)
        {
            YUYV(low_y, high_y, low_x)
        }
        else
        {
            YUYV(0, low_y, low_x)
            YUYV(low_y, high_y, 0)
        }
    }
}
