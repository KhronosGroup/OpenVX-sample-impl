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

static vx_uint8 vx_and_op(vx_uint8 a, vx_uint8 b)
{
    return a & b;
}

static vx_uint8 vx_or_op(vx_uint8 a, vx_uint8 b)
{
    return a | b;
}

static vx_uint8 vx_xor_op(vx_uint8 a, vx_uint8 b)
{
    return a ^ b;
}

void And_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint8 *src_1 = in_1->base[0] + in_1->tile_x;
    vx_uint8 *src_2 = in_2->base[0] + in_2->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
 
    for (y = low_height; y < height; y++) 
    {
        const vx_uint8* src1R = src_1 + y * in_1->image.width;
        const vx_uint8* src2R = src_2 + y * in_2->image.width;
        vx_uint8* dstR = dst + y * out->image.width;
        for (x = 0; x < out->tile_block.width; x+=16) 
        {
            uint8x16_t vSrc1R = vld1q_u8(src1R);
            uint8x16_t vSrc2R = vld1q_u8(src2R);
            uint8x16_t vAnd = vandq_u8(vSrc1R, vSrc2R);
            vst1q_u8(dstR, vAnd);

            src2R += 16;
            src1R += 16;
            dstR += 16;
        }
    }
}


#define vxBinaryU1Op(op)                                                                       \
    void *src_base1 = in_1->base[0];                                                           \
    void *src_base2 = in_2->base[0];                                                           \
    void *dst_base = out->base[0];                                                             \
    vx_uint32 width = vxTileWidth(out, 0) - in_1->rect.start_x % 8;                            \
    vx_uint32 height = vxTileHeight(out, 0);                                                   \
                                                                                               \
    for (y = 0; y < height; y++)                                                               \
    {                                                                                          \
        for (x = 0; x < width; x++)                                                            \
        {                                                                                      \
            vx_uint32 xShftd = x + in_1->rect.start_x % 8;                                     \
            vx_uint8 *src[2] = {                                                               \
                vxFormatImagePatchAddress2d(src_base1, xShftd, y, in_1->addr),                 \
                vxFormatImagePatchAddress2d(src_base2, xShftd, y, in_2->addr),                 \
            };                                                                                 \
            vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, xShftd, y, out->addr);       \
                                                                                               \
            vx_uint8 mask  = 1 << (xShftd % 8);                                                \
            vx_uint8 pixel = op(*src[0], *src[1]) & mask;                                      \
            *dst = (*dst & ~mask) | pixel;                                                     \
        }                                                                                      \
    }



void And_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint8 *src_1 = in_1->base[0] + in_1->tile_x;
    vx_uint8 *src_2 = in_2->base[0] + in_2->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;

    if (in_1->is_U1 == 0)
    {
        if (ty == 0 && tx == 0)
        {
            for (y = 0; y < vxTileHeight(out, 0); y++) 
            {
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)&*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }        
        }
        else
        {
            for (y = 0; y < ty; y++)
            {
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = tx; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)&*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }
            for (y = ty; y < vxTileHeight(out, 0); y++)
            {
                src_1 = in_1->base[0];
                src_2 = in_2->base[0];
                dst = out->base[0];
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)&*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }
        } 
    }
    else
    {
        vxBinaryU1Op(vx_and_op)
    }
}

void Or_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint8 *src_1 = in_1->base[0] + in_1->tile_x;
    vx_uint8 *src_2 = in_2->base[0] + in_2->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
 
    for (y = low_height; y < height; y++) 
    {
        const vx_uint8* src1R = src_1 + y * in_1->image.width;
        const vx_uint8* src2R = src_2 + y * in_2->image.width;
        vx_uint8* dstR = dst + y * out->image.width;
        for (x = 0; x < out->tile_block.width; x+=16) 
        {
            uint8x16_t vSrc1R = vld1q_u8(src1R);
            uint8x16_t vSrc2R = vld1q_u8(src2R);
            uint8x16_t vOr = vorrq_u8(vSrc1R, vSrc2R);
            vst1q_u8(dstR, vOr);

            src2R += 16;
            src1R += 16;
            dstR += 16;
        }
    }
}


void Or_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint8 *src_1 = in_1->base[0] + in_1->tile_x;
    vx_uint8 *src_2 = in_2->base[0] + in_2->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;

    if (in_1->is_U1 == 0)
    {
        if (ty == 0 && tx == 0)
        {
            for (y = 0; y < vxTileHeight(out, 0); y++) 
            {
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)|*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }        
        }
        else
        {
            for (y = 0; y < ty; y++)
            {
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = tx; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)|*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }
            for (y = ty; y < vxTileHeight(out, 0); y++)
            {
                src_1 = in_1->base[0];
                src_2 = in_2->base[0];
                dst = out->base[0];
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)|*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }
        } 
    }
    else
    {
        vxBinaryU1Op(vx_or_op)
    }
}


void Xor_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint8 *src_1 = in_1->base[0] + in_1->tile_x;
    vx_uint8 *src_2 = in_2->base[0] + in_2->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
 
    for (y = low_height; y < height; y++) 
    {
        const vx_uint8* src1R = src_1 + y * in_1->image.width;
        const vx_uint8* src2R = src_2 + y * in_2->image.width;
        vx_uint8* dstR = dst + y * out->image.width;
        for (x = 0; x < out->tile_block.width; x+=16) 
        {
            uint8x16_t vSrc1R = vld1q_u8(src1R);
            uint8x16_t vSrc2R = vld1q_u8(src2R);
            uint8x16_t vXor = veorq_u8(vSrc1R, vSrc2R);
            vst1q_u8(dstR, vXor);

            src2R += 16;
            src1R += 16;
            dstR += 16;
        }
    }
}


void Xor_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    vx_uint8 *src_1 = in_1->base[0] + in_1->tile_x;
    vx_uint8 *src_2 = in_2->base[0] + in_2->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;

    if (in_1->is_U1 == 0)
    {
        if (ty == 0 && tx == 0)
        {
            for (y = 0; y < vxTileHeight(out, 0); y++) 
            {
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)^*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }        
        }
        else
        {
            for (y = 0; y < ty; y++)
            {
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = tx; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)^*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }
            for (y = ty; y < vxTileHeight(out, 0); y++)
            {
                src_1 = in_1->base[0];
                src_2 = in_2->base[0];
                dst = out->base[0];
                const vx_uint8* src1R = src_1 + y * in_1->image.width;
                const vx_uint8* src2R = src_2 + y * in_2->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = *(src1R + x)^*(src2R + x);
                    src2R ++;
                    src1R ++;
                    dstR ++;
                }
            }
        }    
    }
    else
    {
        vxBinaryU1Op(vx_xor_op)
    }
}


void Not_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];
    vx_uint8 *src = in->base[0] + in->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
 
    for (y = low_height; y < height; y++) 
    {
        const vx_uint8* srcR = src + y * in->image.width;
        vx_uint8* dstR = dst + y * out->image.width;
        for (x = 0; x < out->tile_block.width; x+=16) 
        {
            uint8x16_t vSrcR = vld1q_u8(srcR);
            uint8x16_t vNot = vmvnq_u8(vSrcR);
            vst1q_u8(dstR, vNot);

            srcR += 16;
            dstR += 16;
        }
    }
}


void Not_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;    
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];
    vx_uint8 *src = in->base[0] + in->tile_x;
    vx_uint8 *dst = out->base[0] + out->tile_x;
    
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;

    if (in->is_U1 == 0)
    {
        if (ty == 0 && tx == 0)
        {
            for (y = 0; y < vxTileHeight(out, 0); y++) 
            {
                const vx_uint8* srcR = src + y * in->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = ~*(srcR + x);
                    srcR ++;
                    dstR ++;
                }
            }        
        }
        else
        {
            for (y = 0; y < ty; y++)
            {
                const vx_uint8* srcR = src + y * in->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = tx; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = ~*(srcR + x);
                    srcR ++;
                    dstR ++;
                }
            }
            for (y = ty; y < vxTileHeight(out, 0); y++)
            {
                src = in->base[0];
                dst = out->base[0];
                const vx_uint8* srcR = src + y * in->image.width;
                vx_uint8* dstR = dst + y * out->image.width;
                for (x = 0; x < vxTileWidth(out, 0); x++) 
                {            
                    *(dstR+x) = ~*(srcR + x);
                    srcR ++;
                    dstR ++;
                }
            }
        } 
    }
    else
    {
        void *src_base = in->base[0];                                                           
        void *dst_base = out->base[0];                                                             
        vx_uint32 width = vxTileWidth(out, 0) - in->rect.start_x % 8;                            
        vx_uint32 height = vxTileHeight(out, 0);                                                      
                                                                                                   
        for (y = 0; y < height; y++)                                                               
        {                                                                                          
            for (x = 0; x < width; x++)                                                            
            {                                                                                      
                vx_uint32 xShftd = x + in->rect.start_x % 8;                                           
                vx_uint8 *src = vxFormatImagePatchAddress2d(src_base, xShftd, y, in->addr);
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, xShftd, y, out->addr);       
                                                                                                   
                vx_uint8 mask  = 1 << (xShftd % 8);
                vx_uint8 pixel = ~*src & mask;
                *dst = (*dst & ~mask) | pixel;                                                 
            }                                                                                      
        }
    }
}
