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

// nodeless version of the Magnitude kernel
vx_status vxMagnitude(vx_image grad_x, vx_image grad_y, vx_image output)
{
    vx_status status = VX_FAILURE;
    vx_uint32 y, x;
    vx_df_image format = 0;
    vx_uint8 *dst_base   = NULL;
    vx_int16 *src_base_x = NULL;
    vx_int16 *src_base_y = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr_x, src_addr_y;
    vx_rectangle_t rect;
    vx_uint32 value;

    if (grad_x == 0 || grad_y == 0)
        return VX_ERROR_INVALID_PARAMETERS;

    vxQueryImage(output, VX_IMAGE_FORMAT, &format, sizeof(format));
    status = vxGetValidRegionImage(grad_x, &rect);
    vx_map_id map_id_x = 0;
    vx_map_id map_id_y = 0;
    vx_map_id map_id_output = 0;
    status |= vxMapImagePatch(grad_x, &rect, 0, &map_id_x, &src_addr_x, (void **)&src_base_x, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(grad_y, &rect, 0, &map_id_y, &src_addr_y, (void **)&src_base_y, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(output, &rect, 0, &map_id_output, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
    for (y = 0; y < src_addr_x.dim_y; y++)
    {
        vx_uint8 * src_y_ptr_x = (vx_uint8 *)src_base_x + src_addr_x.stride_y * ((src_addr_x.scale_y * y) / VX_SCALE_UNITY);
        vx_uint8 * src_y_ptr_y = (vx_uint8 *)src_base_y + src_addr_y.stride_y * ((src_addr_y.scale_y * y) / VX_SCALE_UNITY);
        vx_uint8 * src_y_ptr_dst = (vx_uint8 *)dst_base + dst_addr.stride_y * ((dst_addr.scale_y * y) / VX_SCALE_UNITY);
        
        vx_int32 roiw8 = src_addr_x.dim_x >= 7 ? src_addr_x.dim_x - 7 : 0;
        for (x = 0; x < roiw8; x+=8)
        {
            vx_int16 *in_x = (vx_int16 *)(src_y_ptr_x + src_addr_x.stride_x * ((src_addr_x.scale_x * x) / VX_SCALE_UNITY));
            vx_int16 *in_y = (vx_int16 *)(src_y_ptr_y + src_addr_y.stride_x * ((src_addr_y.scale_x * x) / VX_SCALE_UNITY));
            int16x8_t in_x16x8 = vld1q_s16(in_x);
            int16x8_t in_y16x8 = vld1q_s16(in_y);            
            if (format == VX_DF_IMAGE_U8)
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
                vx_uint8 *dst1 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * x) / VX_SCALE_UNITY);
                vx_float64 sum1 = vgetq_lane_s32(low_grad.val[0], 0) + vgetq_lane_s32(low_grad.val[1], 0) ;
                value = ((vx_int32)sqrt(sum1))/4;
                *dst1 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_uint8 *dst2 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+1)) / VX_SCALE_UNITY);
                vx_float64 sum2 = vgetq_lane_s32(low_grad.val[0], 1) + vgetq_lane_s32(low_grad.val[1], 1) ;
                value = ((vx_int32)sqrt(sum2))/4;
                *dst2 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_uint8 *dst3 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+2)) / VX_SCALE_UNITY);
                vx_float64 sum3 = vgetq_lane_s32(low_grad.val[0], 2) + vgetq_lane_s32(low_grad.val[1], 2) ;
                value = ((vx_int32)sqrt(sum3))/4;
                *dst3 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_uint8 *dst4 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+3)) / VX_SCALE_UNITY);
                vx_float64 sum4 = vgetq_lane_s32(low_grad.val[0], 3) + vgetq_lane_s32(low_grad.val[1], 3) ;
                value = ((vx_int32)sqrt(sum4))/4;
                *dst4 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_uint8 *dst5 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+4)) / VX_SCALE_UNITY);
                vx_float64 sum5 = vgetq_lane_s32(top_grad.val[0], 0) + vgetq_lane_s32(top_grad.val[1], 0) ;
                value = ((vx_int32)sqrt(sum5))/4;
                *dst5 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_uint8 *dst6 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+5)) / VX_SCALE_UNITY);
                vx_float64 sum6 = vgetq_lane_s32(top_grad.val[0], 1) + vgetq_lane_s32(top_grad.val[1], 1) ;
                value = ((vx_int32)sqrt(sum6))/4;
                *dst6 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_uint8 *dst7 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+6)) / VX_SCALE_UNITY);
                vx_float64 sum7 = vgetq_lane_s32(top_grad.val[0], 2) + vgetq_lane_s32(top_grad.val[1], 2) ;
                value = ((vx_int32)sqrt(sum7))/4;
                *dst7 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
                
                vx_uint8 *dst8 = src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+7)) / VX_SCALE_UNITY);
                vx_float64 sum8 = vgetq_lane_s32(top_grad.val[0], 3) + vgetq_lane_s32(top_grad.val[1], 3) ;
                value = ((vx_int32)sqrt(sum8))/4;
                *dst8 = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
            }
            else if (format == VX_DF_IMAGE_S16)
            {
                vx_uint16 *dst1 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * x) / VX_SCALE_UNITY));
                vx_int16 tmpx1 = vgetq_lane_s16(in_x16x8 ,0);
                vx_int16 tmpy1 = vgetq_lane_s16(in_y16x8 ,0);
                vx_float64 grad1[2] = {(vx_float64)tmpx1*tmpx1, (vx_float64)tmpy1*tmpy1};
                vx_float64 sum1 = grad1[0] + grad1[1];
                value = (vx_int32)(sqrt(sum1) + 0.5);
                *dst1 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_uint16 *dst2 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+1)) / VX_SCALE_UNITY));
                vx_int16 tmpx2 = vgetq_lane_s16(in_x16x8 ,1);
                vx_int16 tmpy2 = vgetq_lane_s16(in_y16x8 ,1);
                vx_float64 grad2[2] = {(vx_float64)tmpx2*tmpx2, (vx_float64)tmpy2*tmpy2};
                vx_float64 sum2 = grad2[0] + grad2[1];
                value = (vx_int32)(sqrt(sum2) + 0.5);
                *dst2 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_uint16 *dst3 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+2)) / VX_SCALE_UNITY));
                vx_int16 tmpx3 = vgetq_lane_s16(in_x16x8 ,2);
                vx_int16 tmpy3 = vgetq_lane_s16(in_y16x8 ,2);
                vx_float64 grad3[2] = {(vx_float64)tmpx3*tmpx3, (vx_float64)tmpy3*tmpy3};
                vx_float64 sum3 = grad3[0] + grad3[1];
                value = (vx_int32)(sqrt(sum3) + 0.5);
                *dst3 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_uint16 *dst4 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+3)) / VX_SCALE_UNITY));
                vx_int16 tmpx4 = vgetq_lane_s16(in_x16x8 ,3);
                vx_int16 tmpy4 = vgetq_lane_s16(in_y16x8 ,3);
                vx_float64 grad4[2] = {(vx_float64)tmpx4*tmpx4, (vx_float64)tmpy4*tmpy4};
                vx_float64 sum4 = grad4[0] + grad4[1];
                value = (vx_int32)(sqrt(sum4) + 0.5);
                *dst4 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_uint16 *dst5 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+4)) / VX_SCALE_UNITY));
                vx_int16 tmpx5 = vgetq_lane_s16(in_x16x8 ,4);
                vx_int16 tmpy5 = vgetq_lane_s16(in_y16x8 ,4);
                vx_float64 grad5[2] = {(vx_float64)tmpx5*tmpx5, (vx_float64)tmpy5*tmpy5};
                vx_float64 sum5 = grad5[0] + grad5[1];
                value = (vx_int32)(sqrt(sum5) + 0.5);
                *dst5 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_uint16 *dst6 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+5)) / VX_SCALE_UNITY));
                vx_int16 tmpx6 = vgetq_lane_s16(in_x16x8 ,5);
                vx_int16 tmpy6 = vgetq_lane_s16(in_y16x8 ,5);
                vx_float64 grad6[2] = {(vx_float64)tmpx6*tmpx6, (vx_float64)tmpy6*tmpy6};
                vx_float64 sum6 = grad6[0] + grad6[1];
                value = (vx_int32)(sqrt(sum6) + 0.5);
                *dst6 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_uint16 *dst7 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+6)) / VX_SCALE_UNITY));
                vx_int16 tmpx7 = vgetq_lane_s16(in_x16x8 ,6);
                vx_int16 tmpy7 = vgetq_lane_s16(in_y16x8 ,6);
                vx_float64 grad7[2] = {(vx_float64)tmpx7*tmpx7, (vx_float64)tmpy7*tmpy7};
                vx_float64 sum7 = grad7[0] + grad7[1];
                value = (vx_int32)(sqrt(sum7) + 0.5);
                *dst7 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
                
                vx_uint16 *dst8 = (vx_uint16 *)(src_y_ptr_dst + dst_addr.stride_x * ((dst_addr.scale_x * (x+7)) / VX_SCALE_UNITY));
                vx_int16 tmpx8 = vgetq_lane_s16(in_x16x8 ,7);
                vx_int16 tmpy8 = vgetq_lane_s16(in_y16x8 ,7);
                vx_float64 grad8[2] = {(vx_float64)tmpx8*tmpx8, (vx_float64)tmpy8*tmpy8};
                vx_float64 sum8 = grad8[0] + grad8[1];
                value = (vx_int32)(sqrt(sum8) + 0.5);
                *dst8 = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
            }
        }
        for (; x < src_addr_x.dim_x; x++)
        {
            vx_int16 *in_x = vxFormatImagePatchAddress2d(src_base_x, x, y, &src_addr_x);
            vx_int16 *in_y = vxFormatImagePatchAddress2d(src_base_y, x, y, &src_addr_y);
            if (format == VX_DF_IMAGE_U8)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_int32 grad[2] = {in_x[0]*in_x[0], in_y[0]*in_y[0]};
                vx_float64 sum = grad[0] + grad[1];
                value = ((vx_int32)sqrt(sum))/4;
                *dst = (vx_uint8)(value > UINT8_MAX ? UINT8_MAX : value);
            }
            else if (format == VX_DF_IMAGE_S16)
            {
                vx_uint16 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                vx_float64 grad[2] = {(vx_float64)in_x[0]*in_x[0], (vx_float64)in_y[0]*in_y[0]};
                vx_float64 sum = grad[0] + grad[1];
                value = (vx_int32)(sqrt(sum) + 0.5);
                *dst = (vx_int16)(value > INT16_MAX ? INT16_MAX : value);
            }
        }
    }
    status |= vxUnmapImagePatch(grad_x, map_id_x);
    status |= vxUnmapImagePatch(grad_y, map_id_y);
    status |= vxUnmapImagePatch(output, map_id_output);
    return status;
}

