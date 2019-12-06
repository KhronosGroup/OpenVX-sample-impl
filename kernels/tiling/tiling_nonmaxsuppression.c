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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>


void NonMaxSuppression_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;
    vx_uint8 mask_data = 0;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *mask = (vx_tile_ex_t *)parameters[1];
    vx_int32 *wsize = (vx_int32*)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;
    vx_df_image format = in->image.format;
    vx_int32 border = *wsize / 2;
    
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
    
    vx_uint32 low_width = out->tile_x;
    vx_uint32 width = out->tile_x + out->tile_block.width;
    
    if(low_height == 0)
    {
        low_height = low_height + border;
    }
    if(height == out->image.height)
    {
        height = height - border;
    }
    if (format == VX_DF_IMAGE_U8)
    {
        vx_uint8 *maskCurr = NULL;
        vx_uint8 *maskLeftTop = NULL;
        uint8x16_t vOne16 = vdupq_n_u8(1);
        uint8x8_t vOne8 = vdup_n_u8(1);
        for (y = low_height; y < height; y++)
        {
            vx_uint8 *srcCurr = (vx_uint8 *)in->base[0] + in->tile_x + y * in->addr[0].stride_y + border;
            vx_uint8 *dstCurr = (vx_uint8 *)out->base[0] + out->tile_x + y * out->addr[0].stride_y + border;
            vx_uint8 *leftTop = (vx_uint8 *)in->base[0] + in->tile_x + (y - border) * in->addr[0].stride_y;
            if (mask->is_Null == 0)  //mask is not NULL
            {
                maskCurr = mask->base[0] + y * mask->addr[0].stride_y + border;
                maskLeftTop = mask->base[0] + (y - border) * mask->addr[0].stride_y;
            }
            if(low_width == 0)
            {
                low_width = low_width + border;
            }
            if(width == out->image.width)
            {
                width = width - border;
            }
            for (x = low_width; x < width; x += 16)
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
                for (vx_uint32 j = 0; j < *wsize; j++)
                {
                    for (vx_uint32 i = 0; i < *wsize; i++)
                    {   
                        if (j == border && i == border)
                            continue;
                        else
                        {
                            vNeighborCurr = vld1q_u8(leftTop + j * in->addr[0].stride_y + i);
                            if (mask->is_Null == 0)  //mask is not NULL
                            {
                                uint8x16_t vMaskNeighborCurr = vld1q_u8(maskLeftTop + j * mask->addr[0].stride_y + i);
                                vMaskNeighborCurr = vsubq_u8(vOne16, vorrq_u8(vMaskNeighborCurr, vMaskCurr));
                                vNeighborCurr = vmulq_u8(vNeighborCurr, vMaskNeighborCurr);
                            }                                      
                            vTempResult = (j < border || (j == border && i < border)) ? vcgeq_u8(vSrcCurr, vNeighborCurr) : vcgtq_u8(vSrcCurr, vNeighborCurr);
                            vFlag = vmulq_u8(vFlag, vTempResult);
                        }                 
                    }
                }
                vDstCurr = vmulq_u8(vFlag, vSrcCurr);
                vst1q_u8((vx_uint8 *)dstCurr, vDstCurr);
                srcCurr += 16;
                dstCurr += 16;
                leftTop += 16;
                if (mask->is_Null == 0)  //mask is not NULL
                {
                    maskCurr += 16;
                    maskLeftTop += 16;                
                }
            }
        }
    }
    else
    {
        vx_int32 border = *wsize / 2;
        for (vx_int32 y = low_height; y < height; y++)
        {
            vx_int32 x = 0;
            for (x = low_width; x < width; x+=8)
            {
                uint8x8_t _mask_8x8_o;
                vx_uint8 *_maskp_o;
                if (mask->is_Null == 0)  //mask is not NULL
                {
                    _maskp_o = (vx_uint8 *)mask->base[0] + y*mask->addr[0].stride_y + x*mask->addr[0].stride_x;
                    _mask_8x8_o = vld1_u8(_maskp_o);
                }
                else
                {
                    _mask_8x8_o = vdup_n_u8(0);
                }
                vx_int16 *val_p = (vx_int16 *)((vx_uint8 *)in->base[0] + y*in->addr[0].stride_y + x*in->addr[0].stride_x);
                vx_int16 *dest = (vx_int16 *)((vx_uint8 *)out->base[0] + y*out->addr[0].stride_y + x*out->addr[0].stride_x);
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
                        vx_int16 *neighbor = (vx_int16 *)((vx_uint8 *)in->base[0] 
                            + (y + j)*in->addr[0].stride_y 
                            + (x + i)*in->addr[0].stride_x);
                        int16x8_t neighbor_val_16x8 = vld1q_s16(neighbor);
                        uint8x8_t _mask_8x8_i;
                        vx_uint8 *_maskp_i;
                        if (mask->is_Null == 0)  //mask is not NULL
                        {
                            _maskp_i = (vx_uint8 *)mask->base[0] + (y + j)*mask->addr[0].stride_y + (x + i)*mask->addr[0].stride_x;
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
        }
    }
}

#define NONMAXSUPPRESSION_FLEXIBLE(low_y, low_x, high_y, high_x, in_tile_x, out_tile_x)\
for (vx_int32 y = low_y; y < high_y; y++)\
{\
	for (vx_int32 x = low_x; x < high_x; x++)\
	{\
		vx_uint8 *_mask;\
		if (mask->is_Null == 0)\
		{\
			_mask = (vx_uint8 *)mask->base[0] + mask->tile_x + y * mask->addr[0].stride_y + x * mask->addr[0].stride_x;\
		}\
		else\
		{\
			_mask = &mask_data;\
		}\
		void *val_p = (vx_uint8 *)in->base[0] + in_tile_x + y * in->addr[0].stride_y + x * in->addr[0].stride_x;\
		void *dest = (vx_uint8 *)out->base[0] + out_tile_x + y * out->addr[0].stride_y + x * out->addr[0].stride_x;\
		vx_int32 src_val = format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)val_p : *(vx_int16 *)val_p;\
		if (*_mask != 0)\
		{\
			if (format == VX_DF_IMAGE_U8)\
				*(vx_uint8 *)dest = (vx_uint8)src_val;\
			else\
				*(vx_int16 *)dest = (vx_int16)src_val;\
		}\
		else\
		{\
			vx_bool flag = 1;\
			for (vx_int32 i = -border; i <= border; i++)\
			{\
				for (vx_int32 j = -border; j <= border; j++)\
				{\
					void *neighbor = (vx_uint8 *)in->base[0] + in_tile_x + (y + j) * in->addr[0].stride_y + (x + i) * in->addr[0].stride_x;\
					if (mask->is_Null == 0)\
					{\
						_mask = (vx_uint8 *)mask->base[0] + mask->tile_x + (y + j) * mask->addr[0].stride_y + (x + i) * mask->addr[0].stride_x;\
					}\
					else\
					{\
						_mask = &mask_data;\
					}\
					vx_int32 neighbor_val = format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)neighbor : *(vx_int16 *)neighbor;\
					if ((*_mask == 0)\
						   && (((j < 0 || (j == 0 && i <= 0)) && (src_val < neighbor_val))\
							  || ((j > 0 || (j == 0 && i > 0)) && (src_val <= neighbor_val))))\
					{\
						flag = 0;\
						break;\
					}\
				}\
				if (flag == 0)\
				{\
					break;\
				}\
			}\
			if (flag)\
			{\
				if (format == VX_DF_IMAGE_U8)\
					*(vx_uint8 *)dest = (vx_uint8)src_val;\
				else\
					*(vx_int16 *)dest = (vx_int16)src_val;\
			}\
			else\
			{\
				if (format == VX_DF_IMAGE_U8)\
					*(vx_uint8 *)dest = 0;\
				else\
					*(vx_int16 *)dest = INT16_MIN;\
			}\
		}\
	}\
}\


void NonMaxSuppression_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;
    vx_uint8 mask_data = 0;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *mask = (vx_tile_ex_t *)parameters[1];
    vx_int32 *wsize = (vx_int32*)parameters[2];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[3];
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;
    vx_df_image format = in->image.format;
    vx_int32 border = *wsize / 2;
    
    if (mask->is_U1 == 0)
    {
        if (ty == 0 && tx == 0)
        {
            NONMAXSUPPRESSION_FLEXIBLE(border, border, (vxTileHeight(out, 0) - border), (vxTileWidth(out, 0) - border), in->tile_x, out->tile_x)
        }
        else
        {
            NONMAXSUPPRESSION_FLEXIBLE(border, tx, ty, (vxTileWidth(out, 0) - border), in->tile_x, out->tile_x)
            NONMAXSUPPRESSION_FLEXIBLE(ty, border, (vxTileHeight(out, 0) - border), (vxTileWidth(out, 0) - border), 0, 0)
        }
    }
    else
    {
        void *src_base = in->base[0];           
        void *mask_base = mask->base[0];                                                   
        void *dst_base = out->base[0];    
        vx_int32 shift_x_u1 = in->rect.start_x % 8;

        vx_uint32 width = vxTileWidth(out, 0);                            
        vx_uint32 height = vxTileHeight(out, 0);  

        for (vx_int32 x = border; x < (width - border); x++)
        {
            for (vx_int32 y = border; y < (height - border); y++)
            {
                vx_uint8 *_mask;
                if (mask->is_Null == 0)
                {
                    _mask = (vx_uint8 *)vxFormatImagePatchAddress2d(mask_base, x + shift_x_u1, y, mask->addr);
                    _mask = (*_mask & (1 << ((x + shift_x_u1) % 8))) != 0 ? _mask : &mask_data;
                }
                else
                {
                    _mask = &mask_data;
                }
                void *val_p = vxFormatImagePatchAddress2d(src_base, x, y, in->addr);
                void *dest  = vxFormatImagePatchAddress2d(dst_base, x, y, out->addr);
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
                            void *neighbor = vxFormatImagePatchAddress2d(src_base, x + i, y + j, in->addr);
                			if (mask->is_Null == 0)
                			{
                				_mask = (vx_uint8 *)vxFormatImagePatchAddress2d(mask_base, x + i + shift_x_u1, y + j, mask->addr);
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
    }
}
