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

#include <stdlib.h>
#include <venum.h>
#include <arm_neon.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

static void nonMaxSuppression_U8(vx_uint8* src,
    vx_uint32 srcWidth,
    vx_uint32 srcHeight,
    vx_uint32 srcStride,
    vx_uint8* mask,
    vx_uint32 maskStride,
    vx_uint8* dst,
    vx_uint32 dstStride,
    vx_int32  windowSize)
{
    vx_uint32 radius = (vx_uint32)windowSize >> 1;
    const vx_uint32 W16 = ((srcWidth - radius) >> 4) << 4;
    const vx_uint32 W8 = ((srcWidth - radius) >> 3) << 3;  
    vx_uint8 *maskCurr = NULL;
    vx_uint8 *maskLeftTop = NULL;
    vx_uint32 x, y;
    uint8x16_t vOne16 = vdupq_n_u8(1);
    uint8x8_t vOne8 = vdup_n_u8(1);
    
    for (y = radius; y < srcHeight - radius; y++)
    {
        vx_uint8 *srcCurr = src + y * srcStride + radius;
        vx_uint8 *dstCurr = dst + y * dstStride + radius;
        vx_uint8 *leftTop = src + (y - radius) * srcStride;
        if (mask)
        {
            maskCurr = mask + y * maskStride + radius;
            maskLeftTop = mask + (y - radius) * maskStride;
        }

        for (x = radius; x < W16; x += 16)
        {
            uint8x16_t vSrcCurr = vld1q_u8(srcCurr); 
            uint8x16_t vDstCurr = vld1q_u8(dstCurr);
            uint8x16_t vMaskCurr = vdupq_n_u8(0);
            if (maskCurr)
            {
                vMaskCurr = vld1q_u8(maskCurr);
            }
            uint8x16_t vNeighborCurr;
            uint8x16_t vTempResult;
            uint8x16_t vFlag = vdupq_n_u8(1);
            for (vx_uint32 j = 0; j < windowSize; j++)
            {
                for (vx_uint32 i = 0; i < windowSize; i++)
                {   
                    if (j == radius && i == radius)
                        continue;
                    else
                    {
                        vNeighborCurr = vld1q_u8(leftTop + j * srcStride + i);
                        if (mask != NULL)
                        {
                            uint8x16_t vMaskNeighborCurr = vld1q_u8(maskLeftTop + j * maskStride + i);
                            vMaskNeighborCurr = vsubq_u8(vOne16, vorrq_u8(vMaskNeighborCurr, vMaskCurr));
                            vNeighborCurr = vmulq_u8(vNeighborCurr, vMaskNeighborCurr);
                        }                                      
                        vTempResult = (j < radius || (j == radius && i < radius)) ? vcgeq_u8(vSrcCurr, vNeighborCurr) : vcgtq_u8(vSrcCurr, vNeighborCurr);
                        vFlag = vmulq_u8(vFlag, vTempResult);
                    }                 
                }
            }
            vDstCurr = vmulq_u8(vFlag, vSrcCurr);
            vst1q_u8((vx_uint8 *)dstCurr, vDstCurr);
            srcCurr += 16;
            dstCurr += 16;
            leftTop += 16;
            if (mask)
            {
                maskCurr += 16;
                maskLeftTop += 16;                
            }
        }
        for (; x < W8; x += 8)
        {
            uint8x8_t vSrcCurr = vld1_u8(srcCurr);
            uint8x8_t vDstCurr = vld1_u8(dstCurr);
            uint8x8_t vMaskCurr = vdup_n_u8(0);
            if (maskCurr)
            {
                vMaskCurr = vld1_u8(maskCurr);
            }
            uint8x8_t vNeighborCurr = vdup_n_u8(0);
            uint8x8_t vTempResult = vdup_n_u8(0);
            uint8x8_t vFlag = vdup_n_u8(1);
            for (vx_uint32 j = 0; j < windowSize; j++)
            {
                for (vx_uint32 i = 0; i < windowSize; i++)
                {
                    if (j == radius && i == radius)
                        continue;
                    else
                    {
                        vNeighborCurr = vld1_u8(leftTop + j * srcStride + i);
                        if (mask != NULL)
                        {
                            uint8x8_t vMaskNeighborCurr = vld1_u8(maskLeftTop + j * maskStride + i);
                            vMaskNeighborCurr = vsub_u8(vOne8, vorr_u8(vMaskNeighborCurr, vMaskCurr));
                            vNeighborCurr = vmul_u8(vNeighborCurr, vMaskNeighborCurr);

                        }
                        vTempResult = (j < radius || (j == radius && i < radius)) ? vcge_u8(vSrcCurr, vNeighborCurr) : vcgt_u8(vSrcCurr, vNeighborCurr);
                        vFlag = vmul_u8(vFlag, vTempResult);
                    }
                }
            }
            vDstCurr = vmul_u8(vFlag, vSrcCurr);
            vst1_u8((vx_uint8 *)dstCurr, vDstCurr);
            srcCurr += 8;
            dstCurr += 8;
            leftTop += 8;
            if (mask)
            {
                maskCurr += 8;
                maskLeftTop += 8;
            }
        }        
        for (;x < srcWidth - radius; x++)
        {
            vx_uint8 maskValue = 0;
            if (mask != NULL) 
            {
                maskValue = *maskCurr;
            }
            else 
            {
                maskValue = 0;
            }
            vx_uint8 srcVal = *srcCurr;
            if (maskValue != 0) 
            {
                *dstCurr = srcVal;
            }
            else 
            {
                vx_uint8 flag = 1;
                for (vx_uint32 j = 0; j < windowSize; j++) 
                {
                    for (vx_uint32 i = 0; i < windowSize; i++) 
                    {
                        if (mask != NULL) 
                        {
                            maskValue = *(maskLeftTop + j * maskStride + i);
                        }
                        else 
                        {
                            maskValue = 0;
                        }
                        vx_uint8 neighborVal = *(leftTop + j * srcStride + i);
                        if ((maskValue == 0) &&
                            (((j < radius || (j == radius && i <= radius)) &&
                            (srcVal < neighborVal))
                            || ((j > radius || (j == radius && i > radius)) &&
                            (srcVal <= neighborVal)))) {
                            flag = 0;
                            break;
                        }
                    }
                    if (flag == 0) 
                    {
                        break;
                    }
                }
                if (flag) {
                    *dstCurr = srcVal;
                }
                else {
                    *dstCurr = 0;
                }
            }
            srcCurr++;
            dstCurr++;
            leftTop++;
            if (mask)
            {
                maskCurr++;
                maskLeftTop++;
            }
        }
    }
}

// nodeless version of the Non Max Suppression kernel
vx_status vxNonMaxSuppression_U8(vx_image input, vx_image mask, vx_scalar win_size, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_int32 height, width;
    vx_uint8 mask_data = 0;
    void *src_base = NULL;
    void *mask_base = NULL;
    void *dst_base = NULL;
    
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t mask_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t src_rect, rect, mask_rect;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_map_id mask_map_id = 0;

    status = vxGetValidRegionImage(output, &rect);
    status |= vxMapImagePatch(output, &rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxGetValidRegionImage(input, &src_rect);
    status |= vxMapImagePatch(input, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    if (mask != NULL)
    {
        status |= vxGetValidRegionImage(mask, &mask_rect);
        status |= vxMapImagePatch(mask, &mask_rect, 0, &mask_map_id, &mask_addr, (void **)&mask_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    }

    vx_df_image format = 0;
    status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
    width = dst_addr.dim_x;
    height = dst_addr.dim_y;
    vx_int32 wsize = 0;
    status |= vxCopyScalar(win_size, &wsize, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);    
    if (format == VX_DF_IMAGE_U8)
    {
        nonMaxSuppression_U8((vx_uint8*)src_base,
            src_addr.dim_x,
            src_addr.dim_y,
            src_addr.stride_y,
            (vx_uint8*)mask_base,
            mask_addr.stride_y,
            (vx_uint8*)dst_base,
            dst_addr.stride_y,
            wsize);
    }
    else
    {
        vx_int32 border = wsize / 2;
        vx_int32 roiw8 = (width - border) >= 7 ? (width - border) - 7 : 0;
        for (vx_int32 y = border; y < (height - border); y++)
        {
            vx_int32 x = border;
            for (; x < roiw8; x+=8)
            {
                uint8x8_t _mask_8x8_o;
                vx_uint8 *_maskp_o;
                if (mask != NULL)
                {
                    _maskp_o = (vx_uint8 *)mask_base + y*mask_addr.stride_y + x*mask_addr.stride_x;
                    _mask_8x8_o = vld1_u8(_maskp_o);
                }
                else
                {
                    _mask_8x8_o = vdup_n_u8(0);
                }
                vx_int16 *val_p = (vx_int16 *)((vx_uint8 *)src_base + y*src_addr.stride_y + x*src_addr.stride_x);
                vx_int16 *dest = (vx_int16 *)((vx_uint8 *)dst_base + y*dst_addr.stride_y + x*dst_addr.stride_x);
                int16x8_t src_val_16x8 = vld1q_s16(val_p);
                
                int16x8_t dst_16x8; 
                uint8x8_t t_8x8 = vdup_n_u8(0);
                uint8x8_t maskequal0_8x8_o = vceq_u8(_mask_8x8_o, vdup_n_u8(0));
                dst_16x8 = vbslq_s16(vmovl_u8(maskequal0_8x8_o), dst_16x8, src_val_16x8);
                t_8x8 = vbsl_u8(maskequal0_8x8_o, t_8x8, vdup_n_u8(1));
                
                for (vx_int32 j = -border; j <= border; j++)
                {
                    for (vx_int32 i = -border; i <= border; i++)
                    {
                        vx_int16 *neighbor = (vx_int16 *)((vx_uint8 *)src_base 
                            + (y + j)*src_addr.stride_y 
                            + (x + i)*src_addr.stride_x);
                        int16x8_t neighbor_val_16x8 = vld1q_s16(neighbor);
                        uint8x8_t _mask_8x8_i;
                        vx_uint8 *_maskp_i;
                        if (mask != NULL)
                        {
                            _maskp_i = (vx_uint8 *)mask_base + (y + j)*mask_addr.stride_y + (x + i)*mask_addr.stride_x;
                            _mask_8x8_i = vld1_u8(_maskp_i);
                        }
                        else
                        {
                            _mask_8x8_i = vdup_n_u8(0);
                        }
                        
                        uint8x8_t maskequal0_8x8_i = vceq_u8(_mask_8x8_i, vdup_n_u8(0));//(*_mask == 0)
                        uint16x8_t j1 = vdupq_n_u16(0);
                        if(j < 0 || (j == 0 && i <= 0))
                        {
                            j1 = vdupq_n_u16(1);
                        }
                        uint16x8_t j2 = vdupq_n_u16(0);
                        if(j > 0 || (j == 0 && i > 0))
                        {
                            j2 = vdupq_n_u16(1);
                        }
                        uint16x8_t srcltval = vcltq_s16(src_val_16x8, neighbor_val_16x8);//<
                        uint16x8_t srclqval = vcleq_s16(src_val_16x8, neighbor_val_16x8);//<=
                        
                        uint16x8_t result_16x8 = vandq_u16(vmovl_u8(maskequal0_8x8_i),
                            vorrq_u16(vandq_u16(j1, srcltval),vandq_u16(j2,srclqval)));
                        if(vgetq_lane_u16(result_16x8, 0) != 0 && vget_lane_u8(t_8x8, 0) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 0);
                            t_8x8 = vset_lane_u8(1, t_8x8, 0);
                        }
                        if(vget_lane_u8(t_8x8, 0) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 0), dst_16x8, 0);
                        }
                    
                        if(vgetq_lane_u16(result_16x8, 1) != 0 && vget_lane_u8(t_8x8, 1) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 1);
                            t_8x8 = vset_lane_u8(1, t_8x8, 1);
                        }
                        if(vget_lane_u8(t_8x8, 1) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 1), dst_16x8, 1);
                        }
                        
                        if(vgetq_lane_u16(result_16x8, 2) != 0 && vget_lane_u8(t_8x8, 2) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 2);
                            t_8x8 = vset_lane_u8(1, t_8x8, 2);
                        }
                        if(vget_lane_u8(t_8x8, 2) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 2), dst_16x8, 2);
                        }
                        
                        if(vgetq_lane_u16(result_16x8, 3) != 0 && vget_lane_u8(t_8x8, 3) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 3);
                            t_8x8 = vset_lane_u8(1, t_8x8, 3);
                        }
                        if(vget_lane_u8(t_8x8, 3) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 3), dst_16x8, 3);
                        }
                        
                        if(vgetq_lane_u16(result_16x8, 4) != 0 && vget_lane_u8(t_8x8, 4) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 4);
                            t_8x8 = vset_lane_u8(1, t_8x8, 4);
                        }
                        if(vget_lane_u8(t_8x8, 4) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 4), dst_16x8, 4);
                        }
                        
                        if(vgetq_lane_u16(result_16x8, 5) != 0 && vget_lane_u8(t_8x8, 5) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 5);
                            t_8x8 = vset_lane_u8(1, t_8x8, 5);
                        }
                        if(vget_lane_u8(t_8x8, 5) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 5), dst_16x8, 5);
                        }
                        
                        if(vgetq_lane_u16(result_16x8, 6) != 0 && vget_lane_u8(t_8x8, 6) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 6);
                            t_8x8 = vset_lane_u8(1, t_8x8, 6);
                        }
                        if(vget_lane_u8(t_8x8, 6) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 6), dst_16x8, 6);
                        }
                        
                        if(vgetq_lane_u16(result_16x8, 7) != 0 && vget_lane_u8(t_8x8, 7) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(INT16_MIN, dst_16x8, 7);
                            t_8x8 = vset_lane_u8(1, t_8x8, 7);
                        }
                        if(vget_lane_u8(t_8x8, 7) ==0)
                        {
                            dst_16x8 = vsetq_lane_s16(vgetq_lane_s16(src_val_16x8, 7), dst_16x8, 7);
                        }
                    }
                }
                vst1q_s16(dest, dst_16x8);
            }

            for (; x < (width - border); x++)
            {
                vx_uint8 *_mask;
                if (mask != NULL)
                {
                    _mask = (vx_uint8 *)mask_base + y*mask_addr.stride_y + x*mask_addr.stride_x;
                }
                else
                {
                    _mask = &mask_data;
                }
                vx_int16 *val_p = (vx_int16 *)((vx_uint8 *)src_base + y*src_addr.stride_y + x*src_addr.stride_x);
                vx_int16 *dest = (vx_int16 *)((vx_uint8 *)dst_base + y*dst_addr.stride_y + x*dst_addr.stride_x);
                vx_int16 src_val = *val_p;
                if (*_mask != 0)
                {
                    *dest = src_val;
                }
                else
                {
                    for (vx_int32 j = -border; j <= border; j++)
                    {
                        for (vx_int32 i = -border; i <= border; i++)
                        {
                            vx_int16 *neighbor = (vx_int16 *)((vx_uint8 *)src_base 
                                + (y + j)*src_addr.stride_y 
                                + (x + i)*src_addr.stride_x);
                            if (mask != NULL)
                            {
                                _mask = (vx_uint8 *)mask_base + (y + j)*mask_addr.stride_y + (x + i)*mask_addr.stride_x;
                            }
                            else
                            {
                                _mask = &mask_data;
                            }
                            vx_int16 neighbor_val = *neighbor;
                            if ((*_mask == 0) && 
                                (((j < 0 || (j == 0 && i <= 0)) && (src_val < neighbor_val)) 
                                    || ((j > 0 || (j == 0 && i > 0)) && (src_val <= neighbor_val))))
                            {
                                *dest = INT16_MIN;
                                i = border + 1;
                                j = border + 1;
                            }
                            else
                            {
                                *dest = src_val;
                            }
                        }
                    }
                }
            }
        }
    }
    status |= vxUnmapImagePatch(input, src_map_id);
    status |= vxUnmapImagePatch(output, dst_map_id);
    if (mask != NULL)
    {
        status |= vxUnmapImagePatch(mask, mask_map_id);
    }
    return status;
}


vx_status vxNonMaxSuppression_U1(vx_image input, vx_image mask, vx_scalar win_size, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_int32 height, width, shift_x_u1 = 0;
    vx_df_image format = 0, mask_format = 0;
    vx_uint8 mask_data = 0;
    void *src_base = NULL;
    void *mask_base = NULL;
    void *dst_base = NULL;

    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t mask_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t src_rect;
    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;
    vx_map_id mask_map_id = 0;

    status  = vxGetValidRegionImage(input, &src_rect);
    status |= vxMapImagePatch(input,  &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY,  VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &src_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    if (mask != NULL)
    {
        status |= vxMapImagePatch(mask, &src_rect, 0, &mask_map_id, &mask_addr, (void **)&mask_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
        status |= vxQueryImage(mask, VX_IMAGE_FORMAT, &mask_format, sizeof(mask_format));
        if (mask_format == VX_DF_IMAGE_U1)
            shift_x_u1 = src_rect.start_x % 8;
    }

    status |= vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

    width  = src_addr.dim_x;
    height = src_addr.dim_y;

    vx_int32 wsize = 0;
    status |= vxCopyScalar(win_size, &wsize, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vx_int32 border = wsize / 2;

    for (vx_int32 x = border; x < (width - border); x++)
    {
        for (vx_int32 y = border; y < (height - border); y++)
        {
            vx_uint8 *_mask;
            if (mask != NULL)
            {
                _mask = (vx_uint8 *)vxFormatImagePatchAddress2d(mask_base, x + shift_x_u1, y, &mask_addr);
                if (mask_format == VX_DF_IMAGE_U1)
                    _mask = (*_mask & (1 << ((x + shift_x_u1) % 8))) != 0 ? _mask : &mask_data;
            }
            else
            {
                _mask = &mask_data;
            }
            void *val_p = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            void *dest  = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
            vx_int32 src_val = format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)val_p : *(vx_int16 *)val_p;
            if (*_mask != 0)
            {
                if (format == VX_DF_IMAGE_U8)
                    *(vx_uint8 *)dest = (vx_uint8)src_val;
                else
                    *(vx_int16 *)dest = (vx_int16)src_val;
            }
            else
            {
                vx_bool flag = 1;
                for (vx_int32 i = -border; i <= border; i++)
                {
                    for (vx_int32 j = -border; j <= border; j++)
                    {
                        void *neighbor = vxFormatImagePatchAddress2d(src_base, x + i, y + j, &src_addr);
            			if (mask != NULL)
            			{
            				_mask = (vx_uint8 *)vxFormatImagePatchAddress2d(mask_base, x + i + shift_x_u1, y + j, &mask_addr);
                            if (mask_format == VX_DF_IMAGE_U1)
                                _mask = (*_mask & (1 << ((x + i + shift_x_u1) % 8))) != 0 ? _mask : &mask_data;
            			}
            			else
            			{
            				_mask = &mask_data;
            			}
                        vx_int32 neighbor_val = format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)neighbor : *(vx_int16 *)neighbor;
                        if ( (*_mask == 0) && (((j < 0 || (j == 0 && i <= 0)) && (src_val <  neighbor_val))
            			                   ||  ((j > 0 || (j == 0 && i >  0)) && (src_val <= neighbor_val))) )
                        {
                            flag = 0;
                            break;
                        }
                    }
                    if (flag == 0)
                    {
                        break;
                    }
                }
                if (flag)
                {
                    if (format == VX_DF_IMAGE_U8)
                        *(vx_uint8 *)dest = (vx_uint8)src_val;
                    else
                        *(vx_int16 *)dest = (vx_int16)src_val;
                }
                else
                {
                    if (format == VX_DF_IMAGE_U8)
                        *(vx_uint8 *)dest = 0;
                    else
                        *(vx_int16 *)dest = INT16_MIN;
                }
            }
        }
    }
    status |= vxUnmapImagePatch(input, src_map_id);
    status |= vxUnmapImagePatch(output, dst_map_id);
    if (mask != NULL)
    {
        status |= vxUnmapImagePatch(mask, mask_map_id);
    }
    return status;
}

vx_status vxNonMaxSuppression(vx_image input, vx_image mask, vx_scalar win_size, vx_image output)
{
    vx_df_image format = 0;
    vx_rectangle_t src_rect;
    vx_status status = VX_SUCCESS;
    if (mask != NULL)
        status = vxQueryImage(mask, VX_IMAGE_FORMAT, &format, sizeof(format));
    status = vxGetValidRegionImage(input, &src_rect);
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1 || src_rect.start_x != 0)
        return vxNonMaxSuppression_U1(input, mask, win_size, output);
    else
        return vxNonMaxSuppression_U8(input, mask, win_size, output);
}
