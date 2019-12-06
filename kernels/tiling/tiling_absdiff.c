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

void AbsDiff_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
    switch (in_1->image.format) {
        case VX_DF_IMAGE_U8:
        {
            for (y = low_height; y < height; y++) {
                const vx_uint8* src1R = (vx_uint8 *)in_1->base[0] + in_1->tile_x + y * in_1->addr[0].stride_y;
                const vx_uint8* src2R = (vx_uint8 *)in_2->base[0] + in_2->tile_x + y * in_2->addr[0].stride_y;
                vx_uint8* dstR = (vx_uint8 *)out->base[0] + out->tile_x + y * out->addr[0].stride_y;;
                for (x = 0; x < out->tile_block.width; x+=16) {
                    uint8x16_t vSrc1R = vld1q_u8(src1R);
                    uint8x16_t vSrc2R = vld1q_u8(src2R);
                    uint8x16_t vDiff = vabdq_u8(vSrc1R, vSrc2R);
                    vst1q_u8(dstR, vDiff);
                    src2R += 16* in_1->addr[0].stride_x;
                    src1R += 16* in_2->addr[0].stride_x;
                    dstR += 16* out->addr[0].stride_x;
                }
            }
        }
        break;

        case VX_DF_IMAGE_S16:
        {
            uint16x8_t vMaxs16 = vdupq_n_u16(0x7FFF);
            for (y = low_height; y < height; y++) {
                const vx_int16* src1R = (vx_int16 *)in_1->base[0] + in_1->tile_x + y * in_1->addr[0].stride_y /2;// + x * in_1->addr[0].stride_x / 2;
                const vx_int16* src2R = (vx_int16 *)in_2->base[0] + in_2->tile_x + y * in_2->addr[0].stride_y /2;// + x * in_2->addr[0].stride_x / 2;
                vx_int16* dstR = (vx_int16 *)out->base[0] + out->tile_x + y * out->addr[0].stride_y /2;// + x * in_1->addr[0].stride_x / 2;
                if (out->image.format == VX_DF_IMAGE_S16) {
                    for (x = 0; x < out->tile_block.width; x+=8) {
                        int16x8_t vSrc1R = vld1q_s16(src1R);
                        int16x8_t vSrc2R = vld1q_s16(src2R);
                        uint16x8_t vDiff = (uint16x8_t)vabdq_s16(vSrc1R, vSrc2R);
                        vDiff = vminq_u16(vDiff, vMaxs16);
                        vst1q_s16(dstR, (int16x8_t)vDiff);
                        src2R += 8 * in_1->addr[0].stride_x / 2;
                        src1R += 8 * in_2->addr[0].stride_x / 2;
                        dstR += 8 * out->addr[0].stride_x / 2;
                    }
                }else if (out->image.format == VX_DF_IMAGE_U16) {
                    for (x = 0; x < out->tile_block.width; x+=8) {
                            int16x8_t vSrc1R = vld1q_s16(src1R);
                            int16x8_t vSrc2R = vld1q_s16(src2R);
                            uint16x8_t vDiff = vabdq_u16((uint16x8_t)vSrc1R, (uint16x8_t)vSrc2R);
                            vst1q_u16((vx_uint16 *)dstR, vDiff);
                            src2R += 8 * in_1->addr[0].stride_x / 2;
                            src1R += 8 * in_2->addr[0].stride_x / 2;
                            dstR += 8 * out->addr[0].stride_x / 2;
                    }
                }
            }
        }
        break;

        case VX_DF_IMAGE_U16:
        {
            for (y = low_height; y < height; y++) {
                const vx_uint16* src1R = (vx_uint16 *)in_1->base[0] + in_1->tile_x + y * in_1->addr[0].stride_y / 2;
                const vx_uint16* src2R =  (vx_uint16 *)in_2->base[0] + in_2->tile_x + y * in_2->addr[0].stride_y / 2;
                vx_uint16* dstR =  (vx_uint16 *)out->base[0] + out->tile_x + y * out->addr[0].stride_y / 2;
                for (x = 0; x < out->tile_block.width; x+=8) {
                    uint16x8_t vSrc1R = vld1q_u16(src1R);
                    uint16x8_t vSrc2R = vld1q_u16(src2R);
                    uint16x8_t vDiff = vabdq_u16(vSrc1R, vSrc2R);
                    vst1q_u16(dstR, vDiff);
                    src2R += 8 * in_1->addr[0].stride_x / 2;
                    src1R += 8 * in_2->addr[0].stride_x / 2;
                    dstR += 8 * out->addr[0].stride_x / 2;
                }
            }
        }
        break;

        default:
            break;
    }
}

#define ABSDIFF_FLEXIBLE(low_y, low_x, high_y, high_x, in_1_tile_x, in_2_tile_x, out_tile_x) \
    switch (in_1->image.format)\
    {\
        case VX_DF_IMAGE_U8:\
            {\
                for (y = low_y; y < high_y; y++) {\
                    vx_uint8* src1R = (vx_uint8 *)in_1->base[0] + in_1_tile_x + y * in_1->addr[0].stride_y;\
                    vx_uint8* src2R = (vx_uint8 *)in_2->base[0] + in_2_tile_x + y * in_2->addr[0].stride_y;\
                    vx_uint8* dstR = (vx_uint8 *)out->base[0] + out_tile_x + y * out->addr[0].stride_y;\
                    for (x = low_x; x < high_x; x++) \
                    {\
                        vx_int16 tmp = (*src1R) - (*src2R);\
                        *dstR = (vx_uint8)(tmp < 0 ? (-tmp) : tmp); \
                        src1R++;\
                        src2R++;\
                        dstR++;\
                    }\
                }\
            }\
            break;\
        default:\
            for (y = low_y; y < high_y; y++)\
            {\
                for (x = low_x; x < high_x; x++)\
                {\
                    if (in_1->image.format == VX_DF_IMAGE_S16)\
                    {\
                        vx_int16 *src[2] = \
                        {\
                            (vx_int16 *)in_1->base[0] + in_1_tile_x + y * in_1->addr[0].stride_y /2 + x * in_1->addr[0].stride_x / 2,\
                            (vx_int16 *)in_2->base[0] + in_2_tile_x + y * in_2->addr[0].stride_y /2 + x * in_2->addr[0].stride_x / 2,\
                        };\
                        if (out->image.format == VX_DF_IMAGE_S16)\
                        {\
                            vx_int16 *dst = (vx_int16 *)out->base[0] + out_tile_x + y * out->addr[0].stride_y / 2 + x * out->addr[0].stride_x / 2;\
                            vx_uint32 val;\
                            if (*src[0] > *src[1])\
                                val = *src[0] - *src[1];\
                            else\
                                val = *src[1] - *src[0];\
                            *dst = (vx_int16)((val > 32767) ? 32767 : val);\
                        }\
                        else if (out->image.format == VX_DF_IMAGE_U16) {\
                            vx_uint16 *dst = (vx_uint16 *)out->base[0] + out_tile_x + y * out->addr[0].stride_y / 2+ x * out->addr[0].stride_x /2;\
                            if (*src[0] > *src[1])\
                                *dst = *src[0] - *src[1];\
                            else\
                                *dst = *src[1] - *src[0];\
                        }\
                    }\
                    else if (in_1->image.format == VX_DF_IMAGE_U16)\
                    {\
                        vx_uint16 *src[2] = \
                        {\
                            (vx_uint16 *)in_1->base[0] + in_1_tile_x + y * in_1->addr[0].stride_y / 2 + x * in_1->addr[0].stride_x / 2,\
                            (vx_uint16 *)in_2->base[0] + in_2->tile_x + y * in_2->addr[0].stride_y / 2 + x * in_2->addr[0].stride_x / 2,\
                        };\
                        vx_uint16 *dst = (vx_uint16 *)out->base[0] + out_tile_x + y * out->addr[0].stride_y + x * out->addr[0].stride_x;\
                        if (*src[0] > *src[1])\
                            *dst = *src[0] - *src[1];\
                        else\
                            *dst = *src[1] - *src[0];\
                    }\
                }\
            }\
            break;\
    }\


void AbsDiff_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;    
    if (ty == 0 && tx == 0)
    {
        ABSDIFF_FLEXIBLE(0, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
    }
    else
    {
        ABSDIFF_FLEXIBLE(0, tx, ty, vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
        ABSDIFF_FLEXIBLE(ty, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), 0, 0, 0)
    }
}

