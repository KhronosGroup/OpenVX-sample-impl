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
#include <math.h>

// nodeless version of the Magnitude kernel
void Magnitude_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x, value;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];    
    vx_uint32 low_height = out->tile_y;
    vx_uint32 height = out->tile_y + out->tile_block.height;
    
    for (y = low_height; y < height; y++)
    {
        vx_int16 *in_x = (vx_int16 *)in_1->base[0] + in_1->tile_x + y * in_1->image.width;
        vx_int16 *in_y = (vx_int16 *)in_2->base[0] + in_2->tile_x + y * in_2->image.width; 
        vx_uint8 *dstp = (vx_uint8 *)out->base[0] + out->tile_x + y * out->image.width; 
        vx_int16 *dstp_16 = (vx_int16 *)out->base[0] + out->tile_x + y * out->image.width; 
        for (x = 0; x < out->tile_block.width; x += 8)
        {
            int16x8_t in_x16x8 = vld1q_s16(in_x);
            int16x8_t in_y16x8 = vld1q_s16(in_y);            
            if (out->image.format == VX_DF_IMAGE_U8)
            {
                const int32x4x2_t low_grad = 
                {
                    {
                      vmovl_s16(vmul_s16(vget_low_s16(in_x16x8), vget_low_s16(in_x16x8))),
                      vmovl_s16(vmul_s16(vget_low_s16(in_y16x8), vget_low_s16(in_y16x8)))
                    }
                };
                const int32x4x2_t top_grad = 
                {
                    {
                      vmovl_s16(vmul_s16(vget_high_s16(in_x16x8), vget_high_s16(in_x16x8))),
                      vmovl_s16(vmul_s16(vget_high_s16(in_y16x8), vget_high_s16(in_y16x8)))
                    }
                };
                
                vx_float64 sum1 = vgetq_lane_s32(low_grad.val[0], 0) + vgetq_lane_s32(low_grad.val[1], 0) ;
                value = ((vx_int32)sqrt(sum1))/4;
                *dstp = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_float64 sum2 = vgetq_lane_s32(low_grad.val[0], 1) + vgetq_lane_s32(low_grad.val[1], 1) ;
                value = ((vx_int32)sqrt(sum2))/4;
                *(dstp+1) = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_float64 sum3 = vgetq_lane_s32(low_grad.val[0], 2) + vgetq_lane_s32(low_grad.val[1], 2) ;
                value = ((vx_int32)sqrt(sum3))/4;
                *(dstp+2) = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_float64 sum4 = vgetq_lane_s32(low_grad.val[0], 3) + vgetq_lane_s32(low_grad.val[1], 3) ;
                value = ((vx_int32)sqrt(sum4))/4;
                *(dstp+3) = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_float64 sum5 = vgetq_lane_s32(top_grad.val[0], 0) + vgetq_lane_s32(top_grad.val[1], 0) ;
                value = ((vx_int32)sqrt(sum5))/4;
                *(dstp+4) = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_float64 sum6 = vgetq_lane_s32(top_grad.val[0], 1) + vgetq_lane_s32(top_grad.val[1], 1) ;
                value = ((vx_int32)sqrt(sum6))/4;
                *(dstp+5) = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_float64 sum7 = vgetq_lane_s32(top_grad.val[0], 2) + vgetq_lane_s32(top_grad.val[1], 2) ;
                value = ((vx_int32)sqrt(sum7))/4;
                *(dstp+6) = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_float64 sum8 = vgetq_lane_s32(top_grad.val[0], 3) + vgetq_lane_s32(top_grad.val[1], 3) ;
                value = ((vx_int32)sqrt(sum8))/4;
                *(dstp+7) = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                dstp += 8;
            }
            else if (out->image.format == VX_DF_IMAGE_S16)
            {
                vx_int16 tmpx1 = vgetq_lane_s16(in_x16x8 ,0);
                vx_int16 tmpy1 = vgetq_lane_s16(in_y16x8 ,0);
                vx_float64 grad1[2] = {(vx_float64)tmpx1*tmpx1, (vx_float64)tmpy1*tmpy1};
                vx_float64 sum1 = grad1[0] + grad1[1];
                value = (vx_int32)(sqrt(sum1) + 0.5);
                *dstp_16 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_int16 tmpx2 = vgetq_lane_s16(in_x16x8 ,1);
                vx_int16 tmpy2 = vgetq_lane_s16(in_y16x8 ,1);
                vx_float64 grad2[2] = {(vx_float64)tmpx2*tmpx2, (vx_float64)tmpy2*tmpy2};
                vx_float64 sum2 = grad2[0] + grad2[1];
                value = (vx_int32)(sqrt(sum2) + 0.5);
                *(dstp_16+1) = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_int16 tmpx3 = vgetq_lane_s16(in_x16x8 ,2);
                vx_int16 tmpy3 = vgetq_lane_s16(in_y16x8 ,2);
                vx_float64 grad3[2] = {(vx_float64)tmpx3*tmpx3, (vx_float64)tmpy3*tmpy3};
                vx_float64 sum3 = grad3[0] + grad3[1];
                value = (vx_int32)(sqrt(sum3) + 0.5);
                *(dstp_16+2) = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_int16 tmpx4 = vgetq_lane_s16(in_x16x8 ,3);
                vx_int16 tmpy4 = vgetq_lane_s16(in_y16x8 ,3);
                vx_float64 grad4[2] = {(vx_float64)tmpx4*tmpx4, (vx_float64)tmpy4*tmpy4};
                vx_float64 sum4 = grad4[0] + grad4[1];
                value = (vx_int32)(sqrt(sum4) + 0.5);
                *(dstp_16+3) = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_int16 tmpx5 = vgetq_lane_s16(in_x16x8 ,4);
                vx_int16 tmpy5 = vgetq_lane_s16(in_y16x8 ,4);
                vx_float64 grad5[2] = {(vx_float64)tmpx5*tmpx5, (vx_float64)tmpy5*tmpy5};
                vx_float64 sum5 = grad5[0] + grad5[1];
                value = (vx_int32)(sqrt(sum5) + 0.5);
                *(dstp_16+4) = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_int16 tmpx6 = vgetq_lane_s16(in_x16x8 ,5);
                vx_int16 tmpy6 = vgetq_lane_s16(in_y16x8 ,5);
                vx_float64 grad6[2] = {(vx_float64)tmpx6*tmpx6, (vx_float64)tmpy6*tmpy6};
                vx_float64 sum6 = grad6[0] + grad6[1];
                value = (vx_int32)(sqrt(sum6) + 0.5);
                *(dstp_16+5) = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_int16 tmpx7 = vgetq_lane_s16(in_x16x8 ,6);
                vx_int16 tmpy7 = vgetq_lane_s16(in_y16x8 ,6);
                vx_float64 grad7[2] = {(vx_float64)tmpx7*tmpx7, (vx_float64)tmpy7*tmpy7};
                vx_float64 sum7 = grad7[0] + grad7[1];
                value = (vx_int32)(sqrt(sum7) + 0.5);
                *(dstp_16+6) = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_int16 tmpx8 = vgetq_lane_s16(in_x16x8 ,7);
                vx_int16 tmpy8 = vgetq_lane_s16(in_y16x8 ,7);
                vx_float64 grad8[2] = {(vx_float64)tmpx8*tmpx8, (vx_float64)tmpy8*tmpy8};
                vx_float64 sum8 = grad8[0] + grad8[1];
                value = (vx_int32)(sqrt(sum8) + 0.5);
                *(dstp_16+7) = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                dstp_16 += 8;
            }
            in_x += 8;
            in_y += 8;
        }
    }
}

#define MAGNITUDE_FLEXIBLE(low_y, low_x, high_y, high_x, in_1_tile_x, in_2_tile_x, out_tile_x)    \
    for (y = low_y; y < high_y; y++)                                                              \
    {                                                                                             \
        vx_int16 *in_x = (vx_int16 *)in_1->base[0] + in_1_tile_x + y * in_1->image.width;        \
        vx_int16 *in_y = (vx_int16 *)in_2->base[0] + in_2_tile_x + y * in_2->image.width;        \
        vx_uint8 *dstp = (vx_uint8 *)out->base[0] + out_tile_x + y * out->image.width;           \
        vx_int16 *dstp_16 = (vx_int16 *)out->base[0] + out_tile_x + y * out->image.width;        \
        for (x = low_x; x < high_x; x++)                                                          \
        {                                                                                         \
            if (out->image.format == VX_DF_IMAGE_U8)                                              \
            {                                                                                     \
                vx_int32 grad[2] = {in_x[0]*in_x[0], in_y[0]*in_y[0]};                            \
                vx_float64 sum = grad[0] + grad[1];                                               \
                value = ((vx_int32)sqrt(sum))/4;                                                  \
                *dstp = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);                        \
                dstp += 1;                                                                        \
            }                                                                                     \
            else if (out->image.format == VX_DF_IMAGE_S16)                                        \
            {                                                                                     \
                vx_float64 grad[2] = {(vx_float64)in_x[0]*in_x[0], (vx_float64)in_y[0]*in_y[0]};  \
                vx_float64 sum = grad[0] + grad[1];                                               \
                value = (vx_int32)(sqrt(sum) + 0.5);                                              \
                *dstp_16 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);                     \
                dstp_16 += 1;                                                                     \
            }                                                                                     \
            in_x += 1;                                                                            \
            in_y += 1;                                                                            \
        }                                                                                         \
    }                                                                                             \
    
void Magnitude_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x, value;    
    vx_tile_ex_t *in_1 = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *in_2 = (vx_tile_ex_t *)parameters[1];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[2];
    
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;
    if (ty == 0 && tx == 0)
    {
        MAGNITUDE_FLEXIBLE(0, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)     
    }
    else
    {
        MAGNITUDE_FLEXIBLE(0, tx, ty, vxTileWidth(out, 0), in_1->tile_x, in_2->tile_x, out->tile_x)
        MAGNITUDE_FLEXIBLE(ty, 0, vxTileHeight(out, 0), vxTileWidth(out, 0), 0, 0, 0)
    }
}
