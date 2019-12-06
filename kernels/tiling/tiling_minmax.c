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

#include <arm_neon.h>
#include <tiling.h>

void Max_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];    
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
    switch (out->image.format)
    {
    case VX_DF_IMAGE_U8:
        for (y = low_height; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)in_1->base[0] + in_1->tile_x + y * in_1->image.width;
            vx_uint8* src1p = (vx_uint8 *)in_2->base[0] + in_2->tile_x + y * in_2->image.width;
            vx_uint8* dstp = (vx_uint8 *)out->base[0] + out->tile_x + y * out->image.width;
            for (x = 0; x < out->tile_block.width; x += 16)
            {
                uint8x16_t vsrc0 = vld1q_u8( src0p + x);
                uint8x16_t vsrc1 = vld1q_u8( src1p + x);
                vst1q_u8( dstp + x, vmaxq_u8( vsrc0, vsrc1 ) );
            }
        }
        break;
    case VX_DF_IMAGE_S16:
        for (y = low_height; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)in_1->base[0] + 2*in_1->tile_x + y * in_1->addr->stride_y;
            vx_uint8* src1p = (vx_uint8 *)in_2->base[0] + 2*in_2->tile_x + y * in_2->addr->stride_y;
            vx_uint8* dstp = (vx_uint8 *)out->base[0] + 2*out->tile_x + y * out->addr->stride_y;
            for (x = 0; x < out->tile_block.width; x += 8)
            {
                int16x8_t vsrc0 = vld1q_s16( (vx_int16 *)(src0p + x * in_1->addr[0].stride_x));
                int16x8_t vsrc1 = vld1q_s16( (vx_int16 *)(src1p + x * in_2->addr[0].stride_x));
                vst1q_s16( (vx_int16 *)(dstp + x * out->addr[0].stride_x), vmaxq_s16( vsrc0, vsrc1 ) );
            }
        }
        break;
    }
}

#define MAX_FLEXIBLE(low_y, low_x, high_y, high_x, in_1_tile_x, in_2_tile_x, out_tile_x)                                   \
    for (y = low_y; y < high_y; ++y)                                                                                       \
    {                                                                                                                      \
        for (x = low_x; x < high_x; ++x)                                                                                   \
        {                                                                                                                  \
            switch (out->image.format)                                                                                     \
            {                                                                                                              \
            case VX_DF_IMAGE_U8:                                                                                           \
                src0p = (vx_uint8 *)in_1->base[0] + in_1_tile_x + y * in_1->image.width + x * in_1->addr[0].stride_x;     \
                src1p = (vx_uint8 *)in_2->base[0] + in_2_tile_x + y * in_2->image.width + x * in_2->addr[0].stride_x;     \
                dstp = (vx_uint8 *)out->base[0] + out_tile_x + y * out->image.width + x * out->addr[0].stride_x;          \
                val0 = *(src0p);                                                                                           \
                val1 = *(src1p);                                                                                           \
                *dstp = val0 > val1 ? val0 : val1;                                                                         \
                break;                                                                                                     \
            case VX_DF_IMAGE_S16:                                                                                          \
                src0p = (vx_uint8 *)in_1->base[0] + 2*in_1_tile_x + y * in_1->addr->stride_y + x * in_1->addr[0].stride_x;\
                src1p = (vx_uint8 *)in_2->base[0] + 2*in_2_tile_x + y * in_2->addr->stride_y + x * in_2->addr[0].stride_x;\
                dstp = (vx_uint8 *)out->base[0] + 2*out_tile_x + y * out->addr->stride_y + x * out->addr[0].stride_x;     \
                val0_16 = *(vx_int16 *)(src0p);                                                                            \
                val1_16 = *(vx_int16 *)(src1p);                                                                            \
                *(vx_int16 *)dstp = val0_16 > val1_16 ? val0_16 : val1_16;                                                 \
                break;                                                                                                     \
            }                                                                                                              \
        }                                                                                                                  \
    }                                                                                                                      \

void Max_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];  
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;
    vx_uint8 *src0p, *src1p, *dstp;
    vx_uint8 val0, val1;
    vx_int16 val0_16, val1_16;
    if (ty == 0 && tx == 0)
    {
        MAX_FLEXIBLE(0, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
    }
    else
    {
        MAX_FLEXIBLE(0, tx, ty, vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
        MAX_FLEXIBLE(ty, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), 0, 0, 0)
    }
}

void Min_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];    
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
    switch (out->image.format)
    {
    case VX_DF_IMAGE_U8:
        for (y = low_height; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)in_1->base[0] + in_1->tile_x + y * in_1->image.width;
            vx_uint8* src1p = (vx_uint8 *)in_2->base[0] + in_2->tile_x + y * in_2->image.width;
            vx_uint8* dstp = (vx_uint8 *)out->base[0] + out->tile_x + y * out->image.width;
            for (x = 0; x < out->tile_block.width; x += 16)
            {
                uint8x16_t vsrc0 = vld1q_u8( src0p + x);
                uint8x16_t vsrc1 = vld1q_u8( src1p + x);
                vst1q_u8( dstp + x, vminq_u8( vsrc0, vsrc1 ) );
            }
        }
        break;
    case VX_DF_IMAGE_S16:
        for (y = low_height; y < height; y++)
        {
            vx_uint8* src0p = (vx_uint8 *)in_1->base[0] + 2*in_1->tile_x + y * in_1->addr->stride_y;
            vx_uint8* src1p = (vx_uint8 *)in_2->base[0] + 2*in_2->tile_x + y * in_2->addr->stride_y;
            vx_uint8* dstp = (vx_uint8 *)out->base[0] + 2*out->tile_x + y * out->addr->stride_y;
            for (x = 0; x < out->tile_block.width; x += 8)
            {
                int16x8_t vsrc0 = vld1q_s16( (vx_int16 *)(src0p + x * in_1->addr[0].stride_x));
                int16x8_t vsrc1 = vld1q_s16( (vx_int16 *)(src1p + x * in_2->addr[0].stride_x));
                vst1q_s16( (vx_int16 *)(dstp + x * out->addr[0].stride_x), vminq_s16( vsrc0, vsrc1 ) );
            }
        }
        break;
    }
}

#define MIN_FLEXIBLE(low_y, low_x, high_y, high_x, in_1_tile_x, in_2_tile_x, out_tile_x)                                   \
    for (y = low_y; y < high_y; ++y)                                                                                       \
    {                                                                                                                      \
        for (x = low_x; x < high_x; ++x)                                                                                   \
        {                                                                                                                  \
            switch (out->image.format)                                                                                     \
            {                                                                                                              \
            case VX_DF_IMAGE_U8:                                                                                           \
                src0p = (vx_uint8 *)in_1->base[0] + in_1_tile_x + y * in_1->image.width + x * in_1->addr[0].stride_x;     \
                src1p = (vx_uint8 *)in_2->base[0] + in_2_tile_x + y * in_2->image.width + x * in_2->addr[0].stride_x;     \
                dstp = (vx_uint8 *)out->base[0] + out_tile_x + y * out->image.width + x * out->addr[0].stride_x;          \
                val0 = *(src0p);                                                                                           \
                val1 = *(src1p);                                                                                           \
                *dstp = val0 < val1 ? val0 : val1;                                                                         \
                break;                                                                                                     \
            case VX_DF_IMAGE_S16:                                                                                          \
                src0p = (vx_uint8 *)in_1->base[0] + 2*in_1_tile_x + y * in_1->addr->stride_y + x * in_1->addr[0].stride_x;\
                src1p = (vx_uint8 *)in_2->base[0] + 2*in_2_tile_x + y * in_2->addr->stride_y + x * in_2->addr[0].stride_x;\
                dstp = (vx_uint8 *)out->base[0] + 2*out_tile_x + y * out->addr->stride_y + x * out->addr[0].stride_x;     \
                val0_16 = *(vx_int16 *)(src0p);                                                                            \
                val1_16 = *(vx_int16 *)(src1p);                                                                            \
                *(vx_int16 *)dstp = val0_16 < val1_16 ? val0_16 : val1_16;                                                 \
                break;                                                                                                     \
            }                                                                                                              \
        }                                                                                                                  \
    }                                                                                                                      \

void Min_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];  
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;
    vx_uint8 *src0p, *src1p, *dstp;
    vx_uint8 val0, val1;
    vx_int16 val0_16, val1_16;
    if (ty == 0 && tx == 0)
    {
        MIN_FLEXIBLE(0, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
    }
    else
    {
        MIN_FLEXIBLE(0, tx, ty, vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
        MIN_FLEXIBLE(ty, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), 0, 0, 0)
    }
}
