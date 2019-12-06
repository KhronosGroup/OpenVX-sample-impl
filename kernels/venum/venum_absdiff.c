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

// nodeless version of the AbsDiff kernel
vx_status vxAbsDiff(vx_image in1, vx_image in2, vx_image output)
{
    vx_uint32 y, x, width = 0, height = 0;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect, r_in1, r_in2;
    vx_df_image format, dst_format;
    vx_status status = VX_SUCCESS;

    vx_map_id in1_map_id    = 0;
    vx_map_id in2_map_id = 0;
    vx_map_id output_map_id = 0;

    vxQueryImage(in1, VX_IMAGE_FORMAT, &format, sizeof(format));
    vxQueryImage(output, VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
    status  = vxGetValidRegionImage(in1, &r_in1);
    status |= vxGetValidRegionImage(in2, &r_in2);
    vxFindOverlapRectangle(&r_in1, &r_in2, &rect);
    status |= vxMapImagePatch(in1, &rect, 0, &in1_map_id, &src_addr[0], (void**)&src_base[0], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(in2, &rect, 0, &in2_map_id, &src_addr[1], (void**)&src_base[1], VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(output, &rect, 0, &output_map_id, &dst_addr, (void**)&dst_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    height = src_addr[0].dim_y;
    width = src_addr[0].dim_x;

    switch (format) {
        case VX_DF_IMAGE_U8:
        {
            vx_uint32 w16 = width >> 4;
            for (y = 0; y < height; y++) {
                const uint8_t* src1R = (uint8_t *)src_base[0] + y * src_addr[0].stride_y;
                const uint8_t* src2R = (uint8_t *)src_base[1] + y * src_addr[0].stride_y;
                uint8_t* dstR = (uint8_t *)dst_base + y * dst_addr.stride_y;
                for (x = 0; x < w16; x++) {
                    uint8x16_t vSrc1R = vld1q_u8(src1R);
                    uint8x16_t vSrc2R = vld1q_u8(src2R);
                    uint8x16_t vDiff = vabdq_u8(vSrc1R, vSrc2R);
                    vst1q_u8(dstR, vDiff);

                    src2R += 16;
                    src1R += 16;
                    dstR += 16;
                }
                int16_t tmp;
                for (x = (w16 << 4); x < width; x++)
                {
                    tmp = (*src1R) - (*src2R);
                    *dstR = (uint8_t)(tmp < 0 ? (-tmp) : tmp);
                    src1R++;
                    src2R++;
                    dstR++;
                }
            }
        }
        break;

        case VX_DF_IMAGE_S16:
        {
            vx_uint32 w8 = width >> 3;
            uint16x8_t vMaxs16 = vdupq_n_u16(0x7FFF);
            for (y = 0; y < height; y++) {
                const int16_t* src1R = (int16_t *)((uint8_t *)src_base[0] + y * src_addr[0].stride_y);
                const int16_t* src2R = (int16_t *)((uint8_t *)src_base[1] + y * src_addr[0].stride_y);
                int16_t* dstR = (int16_t *)((uint8_t *)dst_base + y * dst_addr.stride_y);
                if (dst_format == VX_DF_IMAGE_S16) {
                    for (x = 0; x < w8; x++) {
                        int16x8_t vSrc1R = vld1q_s16(src1R);
                        int16x8_t vSrc2R = vld1q_s16(src2R);
                        uint16x8_t vDiff = (uint16x8_t)vabdq_s16(vSrc1R, vSrc2R);
                        vDiff = vminq_u16(vDiff, vMaxs16);
                        vst1q_s16(dstR, (int16x8_t)vDiff);

                        src2R += 8;
                        src1R += 8;
                        dstR += 8;
                    }
                    int32_t tmp;
                    for (x = (w8 << 3); x < width; x++) {
                        tmp = (*src1R) - (*src2R);
                        tmp = tmp < 0 ? (-tmp) : tmp;
                        *dstR = (vx_int16)((tmp > 0x7FFF) ? 0x7FFF : tmp);

                        src1R++;
                        src2R++;
                        dstR++;
                    }
                }else if (dst_format == VX_DF_IMAGE_U16) {
                    for (x = 0; x < w8; x++) {
                            int16x8_t vSrc1R = vld1q_s16(src1R);
                            int16x8_t vSrc2R = vld1q_s16(src2R);
                            uint16x8_t vDiff = vabdq_u16((uint16x8_t)vSrc1R, (uint16x8_t)vSrc2R);
                            vst1q_u16((uint16_t *)dstR, vDiff);

                            src2R += 8;
                            src1R += 8;
                            dstR += 8;
                    }
                    int32_t tmp;
                    for (x = (w8 << 3); x < width; x++)
                    {
                        tmp = (*src1R) - (*src2R);
                        tmp = tmp < 0 ? (-tmp) : tmp;
                        *(uint16_t *)dstR = (uint16_t)tmp;

                        src1R++;
                        src2R++;
                        dstR++;
                    }
                }
            }
        }
        break;

        case VX_DF_IMAGE_U16:
        {
            vx_uint32 w8 = width >> 3;
            for (y = 0; y < height; y++) {
                const uint16_t* src1R = (uint16_t *)((uint8_t *)src_base[0] + y * src_addr[0].stride_y);
                const uint16_t* src2R = (uint16_t *)((uint8_t *)src_base[1] + y * src_addr[0].stride_y);
                uint16_t* dstR = (uint16_t *)((uint8_t *)dst_base + y * dst_addr.stride_y);
                for (x = 0; x < w8; x++) {
                    uint16x8_t vSrc1R = vld1q_u16(src1R);
                    uint16x8_t vSrc2R = vld1q_u16(src2R);
                    uint16x8_t vDiff = vabdq_u16(vSrc1R, vSrc2R);
                    vst1q_u16(dstR, vDiff);

                    src2R += 8;
                    src1R += 8;
                    dstR += 8;
                }
                int32_t tmp;
                for (x = (w8 << 3); x < width; x++) {
                    tmp = (*src1R) - (*src2R);
                    *dstR = (uint16_t)(tmp < 0 ? (-tmp) : tmp);

                    src1R++;
                    src2R++;
                    dstR++;
                }
            }
        }
        break;

        default:
            break;
    }

    status |= vxUnmapImagePatch(in1, in1_map_id);
    status |= vxUnmapImagePatch(in2, in2_map_id);
    status |= vxUnmapImagePatch(output, output_map_id);

    return status;
}

