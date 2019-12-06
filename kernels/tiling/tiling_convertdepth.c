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

void ConvertDepth_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];
    vx_enum *policy = (vx_enum *)parameters[2];
    vx_int32 *shift = (vx_int32 *)parameters[3];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = out->tile_y + out->tile_block.height;

    if (in->image.format == VX_DF_IMAGE_U8 && out->image.format == VX_DF_IMAGE_S16) 
    {
        vx_uint8 *src_base = in->base[0] + in->tile_x;                                      
        vx_int16 *dst_base = (vx_int16 *)out->base[0] + out->tile_x;   
     
        int16x8_t sh=vdupq_n_s16(*shift);

        for (y = low_y; y < high_y; y++)                
        {
            vx_uint8* srcp = (vx_uint8 *)src_base + y * in->addr->stride_y;                  
            vx_int16* dstp = (vx_int16 *)dst_base + y * out->addr->stride_y / 2;             
            for (x = 0; x < out->tile_block.width; x += 16)
            {
                uint8x16_t v_src = vld1q_u8(srcp);
                int16x8_t v_dst0 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v_src)));
                int16x8_t v_dst1 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(v_src)));

                vst1q_s16(dstp, vshlq_s16(v_dst0, sh));
                vst1q_s16(dstp+8, vshlq_s16(v_dst1, sh));

                srcp+=16;
                dstp+=16;
            }
        } 
    }
    else if (in->image.format == VX_DF_IMAGE_S16 && out->image.format == VX_DF_IMAGE_U8)    
    {                                                                                       
        vx_int16 *src_base = (vx_int16 *)in->base[0] + in->tile_x;                          
        vx_uint8 *dst_base = out->base[0] + out->tile_x;    

        int16x8_t sh=vdupq_n_s16(-(*shift));

        for (y = low_y; y < high_y; y++)                
        {
            vx_int16* srcp = (vx_int16 *)src_base + y * in->addr->stride_y / 2;                  
            vx_uint8* dstp = (vx_uint8 *)dst_base + y * out->addr->stride_y;             
            for (x = 0; x < out->tile_block.width; x += 16)
            {
                int16x8_t v_src0 = vld1q_s16(srcp);
                int16x8_t v_src1 = vld1q_s16(srcp+8);

                if (*policy == VX_CONVERT_POLICY_SATURATE)
                {
                    int16x8_t v_dst0= vqshlq_s16(v_src0,sh);
                    int16x8_t v_dst1= vqshlq_s16(v_src1,sh);
                    uint8x8_t v_dst00 = vqmovun_s16(v_dst0);
                    uint8x8_t v_dst01 = vqmovun_s16(v_dst1);
                    uint8x16_t v_dst = vcombine_u8(v_dst00,v_dst01);

                    vst1q_u8(dstp, v_dst);
                }
                else if (*policy == VX_CONVERT_POLICY_WRAP)
                {
                    int16x8_t v_dst0= vshlq_s16(v_src0,sh);
                    int16x8_t v_dst1= vshlq_s16(v_src1,sh);
                    uint8x16_t v_dst = vcombine_u8(vmovn_u16(vreinterpretq_u16_s16(v_dst0)),vmovn_u16(vreinterpretq_u16_s16(v_dst1)));

                    vst1q_u8(dstp, v_dst);
                }
                srcp+=16;
                dstp+=16;
            }
        } 
    }                                
}


#define CONVERT_DEPTH(low_y, high_y, low_x, in_tile_x, out_tile_x)                          \
    if (in->image.format == VX_DF_IMAGE_U8 && out->image.format == VX_DF_IMAGE_S16)         \
    {                                                                                       \
        vx_uint8 *src_base = in->base[0] + in_tile_x;                                       \
        vx_int16 *dst_base = (vx_int16 *)out->base[0] + out_tile_x;                         \
        for (y = low_y; y < high_y; y++)                                                    \
        {                                                                                   \
            vx_uint8 *src = (vx_uint8 *)src_base + y * in->addr->stride_y;                  \
            vx_int16 *dst = (vx_int16 *)dst_base + y * out->addr->stride_y / 2;             \
            for (x = low_x; x < high_x; x++)                                                \
            {                                                                               \
                *dst = ((vx_int16)(*src)) << (*shift);                                      \
                                                                                            \
                src++;                                                                      \
                dst++;                                                                      \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
    else if (in->image.format == VX_DF_IMAGE_S16 && out->image.format == VX_DF_IMAGE_U8)    \
    {                                                                                       \
        vx_int16 *src_base = (vx_int16 *)in->base[0] + in_tile_x;                           \
        vx_uint8 *dst_base = out->base[0] + out_tile_x;                                     \
        for (y = low_y; y < high_y; y++)                                                    \
        {                                                                                   \
            vx_int16 *src = (vx_int16 *)src_base + y * in->addr->stride_y / 2;              \
            vx_uint8 *dst = (vx_uint8 *)dst_base + y * out->addr->stride_y;                 \
            for (x = low_x; x < high_x; x++)                                                \
            {                                                                               \
                if (*policy == VX_CONVERT_POLICY_WRAP)                                      \
                {                                                                           \
                    *dst = (vx_uint8)((*src) >> (*shift));                                  \
                                                                                            \
                    src++;                                                                  \
                    dst++;                                                                  \
                }                                                                           \
                else if (*policy == VX_CONVERT_POLICY_SATURATE)                             \
                {                                                                           \
                    vx_int16 value = (*src) >> (*shift);                                    \
                    value = (value < 0 ? 0 : value);                                        \
                    value = (value > UINT8_MAX ? UINT8_MAX : value);                        \
                    *dst = (vx_uint8)value;                                                 \
                                                                                            \
                    src++;                                                                  \
                    dst++;                                                                  \
                }                                                                           \
            }                                                                               \
        }                                                                                   \
    }


void ConvertDepth_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];
    vx_enum *policy = (vx_enum *)parameters[2];
    vx_int32 *shift = (vx_int32 *)parameters[3];

    vx_uint32 low_y = out->tile_y;
    vx_uint32 high_y = vxTileHeight(out, 0);

    vx_uint32 low_x = out->tile_x;
    vx_uint32 high_x = vxTileWidth(out, 0);

    if (in->is_U1 == 0 && out->is_U1 == 0)
    {
        if (low_y == 0 && low_x == 0)
        {
            CONVERT_DEPTH(low_y, high_y, low_x, in->tile_x, out->tile_x)
        }
        else
        {
            CONVERT_DEPTH(0, low_y, low_x, in->tile_x, out->tile_x)
            CONVERT_DEPTH(low_y, high_y, 0, 0, 0)
        }
    }
    else
    {
        void *src_base = in->base[0];
        void *dst_base = out->base[0];
        high_x = (in->image.format == VX_DF_IMAGE_U1) ? high_x - in->rect.start_x % 8 : high_x;
        for (y = low_y; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x++)
            {
                if ((in->image.format == VX_DF_IMAGE_U1) && (out->image.format == VX_DF_IMAGE_U8))
                {
                    vx_uint32 xSrc = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, xSrc, y, in->addr);
                    vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);
                    *dst = ((*src & (1 << (xSrc % 8))) != 0 ? 255u : 0u);
                }
                if ((in->image.format == VX_DF_IMAGE_U1) && (out->image.format == VX_DF_IMAGE_U16))
                {
                    vx_uint32 xSrc = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, xSrc, y, in->addr);
                    vx_uint16 *dst = (vx_uint16*)vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);
                    *dst = ((vx_uint16)(*src & (1 << (xSrc % 8))) != 0 ? 65535u : 0u);
                }
                if ((in->image.format == VX_DF_IMAGE_U1) && (out->image.format == VX_DF_IMAGE_S16))
                {
                    vx_uint32 xSrc = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, xSrc, y, in->addr);
                    vx_int16  *dst = (vx_int16*)vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);
                    *dst = ((vx_int16)(*src & (1 << (xSrc % 8))) != 0 ? -1 : 0);
                }
                if ((in->image.format == VX_DF_IMAGE_U1) && (out->image.format == VX_DF_IMAGE_U32))
                {
                    vx_uint32 xSrc = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, xSrc, y, in->addr);
                    vx_uint32 *dst = (vx_uint32*)vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);
                    *dst = ((vx_uint32)(*src & (1 << (xSrc % 8))) != 0 ? 4294967295u : 0u);
                }
                if ((in->image.format == VX_DF_IMAGE_U8) && (out->image.format == VX_DF_IMAGE_U1))
                {
                    vx_uint32 xDst = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_uint8  *src = (vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);
                    vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xDst, y, out->addr);
                    *dst = (*dst & ~(1 << (xDst % 8))) | ((*src != 0 ? 1u : 0u) << (xDst % 8));
                }
                else if ((in->image.format == VX_DF_IMAGE_U16) && (out->image.format == VX_DF_IMAGE_U1))
                {
                    vx_uint32 xDst = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_uint16 *src = (vx_uint16*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);
                    vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xDst, y, out->addr);
                    *dst = (*dst & ~(1 << (xDst % 8))) | ((vx_uint8)(*src != 0 ? 1u : 0u) << (xDst % 8));
                }
                else if ((in->image.format == VX_DF_IMAGE_S16) && (out->image.format == VX_DF_IMAGE_U1))
                {
                    vx_uint32 xDst = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_int16  *src = (vx_int16*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);
                    vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xDst, y, out->addr);
                    *dst = (*dst & ~(1 << (xDst % 8))) | ((vx_uint8)(*src != 0 ? 1u : 0u) << (xDst % 8));
                }
                else if ((in->image.format == VX_DF_IMAGE_U32) && (out->image.format == VX_DF_IMAGE_U1))
                {
                    vx_uint32 xDst = x + in->rect.start_x % 8;   // U1 valid region offset
                    vx_uint32 *src = (vx_uint32*)vxFormatImagePatchAddress2d(src_base, x, y, in->addr);
                    vx_uint8  *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, xDst, y, out->addr);
                    *dst = (*dst & ~(1 << (xDst % 8))) | ((vx_uint8)(*src != 0 ? 1u : 0u) << (xDst % 8));
                }
            }
        }
    }
}
