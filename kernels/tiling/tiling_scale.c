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

static vx_bool read_pixel(void *base, vx_imagepatch_addressing_t *addr, vx_uint32 src_height, vx_uint32 src_width,
        vx_int32 x, vx_int32 y, vx_uint8 *pixel, vx_border_t borders)
{
    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= src_width || y >= src_height);
    vx_uint32 bx, by;
    vx_uint8 *bpixel;
    if (out_of_bounds)
    {
        if (borders.mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders.mode == VX_BORDER_CONSTANT)
        {
            *pixel = borders.constant_value.U8;
            return vx_true_e;
        }
    }

    bx = x < 0 ? 0 : x >= src_width ? src_width - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= src_height ? src_height - 1 : (vx_uint32)y;

    vx_uint8 *new_ptr = NULL;
    vx_uint32 offset = (addr->stride_y * by + addr->stride_x * bx);
    new_ptr = (vx_uint8*)base;
    bpixel = &new_ptr[offset];
    
    *pixel = *bpixel;

    return vx_true_e;
}

static void read_pixel_v(void *base, vx_imagepatch_addressing_t *addr, vx_uint32 src_height, vx_uint32 src_width,
        int32x4_t x_32x4, int32x4_t y_32x4, vx_uint8 *dst, vx_border_t borders)
{
    int32x4_t o_32x4 = vdupq_n_s32(0);
    int32x4_t dimx_32x4 = vdupq_n_s32((vx_int32)src_width);
    int32x4_t dimy_32x4 = vdupq_n_s32((vx_int32)src_height);
    uint32x4_t out_of_bounds_32x4 = vorrq_u32(vorrq_u32(vcltq_s32(x_32x4, o_32x4), vcltq_s32(y_32x4, o_32x4)), 
    vorrq_u32(vcgeq_s32(x_32x4, dimx_32x4),vcgeq_s32(y_32x4, dimy_32x4)));
    
    char flag_1 = 0;
    char flag_2 = 0;
    char flag_3 = 0;
    char flag_4 = 0;
    if(vgetq_lane_u32(out_of_bounds_32x4, 0) == 0xFFFFFFFF)
    {
        if (borders.mode == VX_BORDER_UNDEFINED)
        {
            flag_1 = 1;
        }
        if (flag_1 == 0 && borders.mode == VX_BORDER_CONSTANT && dst)
        {
           *dst = borders.constant_value.U8;
           flag_1 = 1;
        }
    }
    if(vgetq_lane_u32(out_of_bounds_32x4, 1) == 0xFFFFFFFF)
    {
        if (borders.mode == VX_BORDER_UNDEFINED)
        {
            flag_2 = 1;
        }
        if (flag_2 == 0 && borders.mode == VX_BORDER_CONSTANT && (dst+1))
        {
           *(dst+1) = borders.constant_value.U8;
           flag_2 = 1;
        }
    }
    if(vgetq_lane_u32(out_of_bounds_32x4, 2) == 0xFFFFFFFF)
    {
        if (borders.mode == VX_BORDER_UNDEFINED)
        {
            flag_3 = 1;
        }
        if (flag_3 == 0 && borders.mode == VX_BORDER_CONSTANT && (dst+2))
        {
           *(dst+2) = borders.constant_value.U8;
           flag_3 = 1;
        }
    }
    if(vgetq_lane_u32(out_of_bounds_32x4, 3) == 0xFFFFFFFF)
    {
        if (borders.mode == VX_BORDER_UNDEFINED)
        {
            flag_4 = 1;
        }
        if (flag_4 == 0 && borders.mode == VX_BORDER_CONSTANT && (dst+3))
        {
           *(dst+3) = borders.constant_value.U8;
           flag_4 = 1;
        }
    }

    vx_uint8 *bpixel = NULL;
    vx_uint8 *new_ptr = NULL;
    vx_uint32 offset = 0;
    if(flag_1 == 0)
    {
        offset = (addr->stride_y * vgetq_lane_s32(y_32x4, 0) + vgetq_lane_s32(x_32x4, 0));
        new_ptr = (vx_uint8*)base;
        bpixel = &new_ptr[offset];
        *dst = *bpixel;
    }
    if(flag_2 == 0)
    {
        offset = (addr->stride_y * vgetq_lane_s32(y_32x4, 1) + vgetq_lane_s32(x_32x4, 1));
        new_ptr = (vx_uint8*)base;
        bpixel = &new_ptr[offset];
        *(dst+1) = *bpixel;
    }
    if(flag_3 == 0)
    {
        offset = (addr->stride_y * vgetq_lane_s32(y_32x4, 2) + vgetq_lane_s32(x_32x4, 2));
        new_ptr = (vx_uint8*)base;
        bpixel = &new_ptr[offset];
        *(dst+2) = *bpixel;
    }
    if(flag_4 == 0)
    {
        offset = (addr->stride_y * vgetq_lane_s32(y_32x4, 3) + vgetq_lane_s32(x_32x4, 3));
        new_ptr = (vx_uint8*)base;
        bpixel = &new_ptr[offset];
        *(dst+3) = *bpixel;
    }
}

static vx_bool read_pixel_16s(void *base, vx_imagepatch_addressing_t *addr, vx_uint32 src_height, vx_uint32 src_width,
    vx_int32 x, vx_int32 y, vx_int16 *pixel, vx_border_t borders)
{
    vx_uint32 bx;
    vx_uint32 by;
    vx_int16* bpixel;

    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= src_width || y >= src_height);

    if (out_of_bounds)
    {
        if (borders.mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders.mode == VX_BORDER_CONSTANT)
        {
            *pixel = (vx_int16)borders.constant_value.S16;
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < 0 ? 0 : src_width ? src_width - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= src_height ? src_height - 1 : (vx_uint32)y;

    vx_int16 *new_ptr = NULL;
    vx_uint32 offset = (addr->stride_y * by + addr->stride_x * bx);
    new_ptr = (vx_int16*)base;
    bpixel = &new_ptr[offset];
    
    *pixel = *bpixel;

    return vx_true_e;
}

static vx_bool read_pixel_1u(void *base, vx_imagepatch_addressing_t *addr, vx_uint32 src_height, vx_uint32 src_width,
    vx_int32 x, vx_int32 y, vx_uint8 *pixel, vx_uint32 shift_x_u1, vx_border_t borders)
{
    vx_bool out_of_bounds = (x <  (vx_int32)shift_x_u1 || y <  0 || x >= src_width || y >= src_height);
    vx_uint32 bx, by;
    vx_uint8 *bpixel_group;
    if (out_of_bounds)
    {
        if (borders.mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders.mode == VX_BORDER_CONSTANT)
        {
            *pixel = (vx_uint8)(borders.constant_value.U1 ? 1 : 0);
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < (vx_int32)shift_x_u1 ? shift_x_u1 : x >= src_width ? src_width - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= src_height ? src_height - 1 : (vx_uint32)y;

    vx_uint8 *new_ptr = NULL;
    vx_uint32 offset = ((addr->stride_y * ((addr->scale_y * by) / VX_SCALE_UNITY)) +
                       (addr->stride_x_bits * ((addr->scale_x * bx) / VX_SCALE_UNITY)) / 8u);
    new_ptr = (vx_uint8*)base;
    bpixel_group = &new_ptr[offset];
    *pixel = (*bpixel_group & (1 << (bx % 8))) >> (bx % 8);

    return vx_true_e;
}

static void vxNearestScaling_fast(vx_tile_ex_t *src_image, vx_tile_ex_t *dst_image)
{
    vx_int32 x1,y1,x2,y2;
    vx_rectangle_t src_rect, dst_rect;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
    vx_float32 wr, hr;
    vx_df_image format = 0;

    w1 = src_image->image.width;
    h1 = src_image->image.height;
    format = src_image->image.format;
    w2 = dst_image->image.width;
    h2 = dst_image->image.height;
    
    src_image->rect.start_x = src_rect.start_y = 0;
    src_rect.end_x = w1;
    src_rect.end_y = h1;

    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    wr = (vx_float32)w1/(vx_float32)w2;
    hr = (vx_float32)h1/(vx_float32)h2;

    vx_border_t borders = src_image->border;
    
    vx_uint32 low_height = dst_image->tile_y;
    vx_uint32 height = dst_image->tile_y + dst_image->tile_block.height;
    
    vx_uint32 low_width = dst_image->tile_x;
    vx_uint32 width = dst_image->tile_x + dst_image->tile_block.width;
    
    float32x4_t fv_0_5_32x4 = vdupq_n_f32(0.5f);
    float32x4_t fv_wr_32x4 = vdupq_n_f32(wr);
    float32x4_t fv_hr_32x4 = vdupq_n_f32(hr);
    for (y2 = low_height; y2 < height; y2++)
    {
        vx_uint8* dst_u8 = (vx_uint8 *)dst_image->base[0] + dst_image->tile_x + y2 * dst_image->addr[0].stride_y;
        vx_int16* dst_base_s16 = (vx_int16*)dst_image->base[0] + dst_image->tile_x + y2 * dst_image->addr[0].stride_y/2;
        float32x4_t y2_32x4 = vdupq_n_f32((float32_t)y2);
        float32x4_t y_src_32x4 = vsubq_f32(vmulq_f32(vaddq_f32(y2_32x4,fv_0_5_32x4), fv_hr_32x4), fv_0_5_32x4);
        
        int32x4_t y1_32x4 = vcvtq_s32_f32(vaddq_f32(y_src_32x4, fv_0_5_32x4));        
        for (x2 = low_width; x2 < width; x2 += 8)
        {           
            float32_t arr_int32[4]={(float32_t)x2, (float32_t)(x2+1), (float32_t)(x2+2), (float32_t)(x2+3)};
            float32x4_t x2_32x4 = vld1q_f32(arr_int32);            
            float32x4_t x_src_32x4 = vsubq_f32(vmulq_f32(vaddq_f32(x2_32x4,fv_0_5_32x4), fv_wr_32x4), fv_0_5_32x4);
            
            arr_int32[0] = (float32_t)(x2+4);
            arr_int32[1] = (float32_t)(x2+5);
            arr_int32[2] = (float32_t)(x2+6);
            arr_int32[3] = (float32_t)(x2+7);
            float32x4_t x2_32x4_1 = vld1q_f32(arr_int32);
            float32x4_t x_src_32x4_1 = vsubq_f32(vmulq_f32(vaddq_f32(x2_32x4_1,fv_0_5_32x4), fv_wr_32x4), fv_0_5_32x4);
            int32x4_t x1_32x4 = vcvtq_s32_f32(vaddq_f32(x_src_32x4, fv_0_5_32x4));
            int32x4_t x1_32x4_1 = vcvtq_s32_f32(vaddq_f32(x_src_32x4_1, fv_0_5_32x4));
            
            if (VX_DF_IMAGE_U8 == format)
            {
                read_pixel_v((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, x1_32x4, y1_32x4, dst_u8, borders);
                read_pixel_v((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, x1_32x4_1, y1_32x4, (dst_u8+4), borders);
                dst_u8 += 8;
            }
            else
            {
                vx_int16 v = 0;
                vx_int16* dst = dst_base_s16 + dst_image->addr[0].stride_x/2*x2;
                if (dst && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4, 0),vgetq_lane_s32(y1_32x4, 0),&v, borders))
                    *dst = v;
                v=0;
                if ((dst+1) && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4, 1),vgetq_lane_s32(y1_32x4, 1),&v, borders))
                    *(dst+1) = v;
                v=0;
                if ((dst+2) && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4, 2),vgetq_lane_s32(y1_32x4, 2),&v, borders))
                    *(dst+2) = v;
                v=0;
                if ((dst+3) && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4, 3),vgetq_lane_s32(y1_32x4, 3),&v, borders))
                    *(dst+3) = v;
                    
                if ((dst+4) && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4_1, 0),vgetq_lane_s32(y1_32x4, 0),&v, borders))
                    *(dst+4) = v;
                v=0;
                if ((dst+5) && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4_1, 1),vgetq_lane_s32(y1_32x4, 1),&v, borders))
                    *(dst+5) = v;
                v=0;
                if ((dst+6) && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4_1, 2),vgetq_lane_s32(y1_32x4, 2),&v, borders))
                    *(dst+6) = v;
                v=0;
                if ((dst+7) && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0] + src_image->tile_x, src_image->addr,h1, w1,vgetq_lane_s32(x1_32x4_1, 3),vgetq_lane_s32(y1_32x4, 3),&v, borders))
                    *(dst+7) = v;
            }
        }
    }
}

static void vxBilinearScaling_fast(vx_tile_ex_t *src_image, vx_tile_ex_t *dst_image)
{
    vx_int32 x2,y2;
    vx_rectangle_t src_rect, dst_rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
    vx_float32 wr, hr;

    w1 = src_image->image.width;
    h1 = src_image->image.height;
    w2 = dst_image->image.width;
    h2 = dst_image->image.height;

    src_image->rect.start_x = src_rect.start_y = 0;
    src_rect.end_x = w1;
    src_rect.end_y = h1;

    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    wr = (vx_float32)w1/(vx_float32)w2;
    hr = (vx_float32)h1/(vx_float32)h2;

    vx_border_t borders = src_image->border;
    
    vx_uint32 low_height = dst_image->tile_y;
    vx_uint32 height = dst_image->tile_y + dst_image->tile_block.height;
    
    vx_uint32 low_width = dst_image->tile_x;
    vx_uint32 width = dst_image->tile_x + dst_image->tile_block.width;
    
    for (y2 = low_height; y2 < height; y2++)
    {
        for (x2 = low_width; x2 < width; x2 += 8)
        {
            vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;
            vx_uint8* dst = (vx_uint8 *)dst_image->base[0] + y2 * dst_image->addr[0].stride_y + dst_image->addr[0].stride_x*x2;
            float32x4_t x2_32x4;
            x2_32x4 = vsetq_lane_f32((vx_float32)x2, x2_32x4, 0);
            x2_32x4 = vsetq_lane_f32((vx_float32)(x2+1), x2_32x4, 1);
            x2_32x4 = vsetq_lane_f32((vx_float32)(x2+2), x2_32x4, 2);
            x2_32x4 = vsetq_lane_f32((vx_float32)(x2+3), x2_32x4, 3);
            float32x4_t x_src_32x4 = vsubq_f32(vmulq_f32(vaddq_f32(x2_32x4,vdupq_n_f32(0.5f)), vdupq_n_f32(wr)),vdupq_n_f32(0.5f));
            
            float32x4_t x2_32x4_1;
            x2_32x4_1 = vsetq_lane_f32((vx_float32)(x2+4), x2_32x4_1, 0);
            x2_32x4_1 = vsetq_lane_f32((vx_float32)(x2+5), x2_32x4_1, 1);
            x2_32x4_1 = vsetq_lane_f32((vx_float32)(x2+6), x2_32x4_1, 2);
            x2_32x4_1 = vsetq_lane_f32((vx_float32)(x2+7), x2_32x4_1, 3);
            float32x4_t x_src_32x4_1 = vsubq_f32(vmulq_f32(vaddq_f32(x2_32x4_1,vdupq_n_f32(0.5f)), vdupq_n_f32(wr)),vdupq_n_f32(0.5f));
            
            float32x4_t y2_32x4 = vdupq_n_f32((vx_float32)y2);
            float32x4_t y_src_32x4 = vsubq_f32(vmulq_f32(vaddq_f32(y2_32x4,vdupq_n_f32(0.5f)), vdupq_n_f32(hr)),vdupq_n_f32(0.5f));
            
            float32x4_t x_min_32x4;
            x_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4, 0)), x_min_32x4, 0);
            x_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4, 1)), x_min_32x4, 1);
            x_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4, 2)), x_min_32x4, 2);
            x_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4, 3)), x_min_32x4, 3);
            
            float32x4_t x_min_32x4_1;
            x_min_32x4_1 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4_1, 0)), x_min_32x4_1, 0);
            x_min_32x4_1 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4_1, 1)), x_min_32x4_1, 1);
            x_min_32x4_1 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4_1, 2)), x_min_32x4_1, 2);
            x_min_32x4_1 = vsetq_lane_f32(floorf(vgetq_lane_f32(x_src_32x4_1, 3)), x_min_32x4_1, 3);
            
            float32x4_t y_min_32x4;
            y_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(y_src_32x4, 0)), y_min_32x4, 0);
            y_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(y_src_32x4, 1)), y_min_32x4, 1);
            y_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(y_src_32x4, 2)), y_min_32x4, 2);
            y_min_32x4 = vsetq_lane_f32(floorf(vgetq_lane_f32(y_src_32x4, 3)), y_min_32x4, 3);
            
            float32x4_t s_32x4 = vsubq_f32(x_src_32x4, x_min_32x4);
            float32x4_t s_32x4_1 = vsubq_f32(x_src_32x4_1, x_min_32x4_1);    
            
            float32_t s_0 = vgetq_lane_f32(s_32x4, 0);
            float32_t s_1 = vgetq_lane_f32(s_32x4, 1);
            float32_t s_2 = vgetq_lane_f32(s_32x4, 2);
            float32_t s_3 = vgetq_lane_f32(s_32x4, 3);
            
            float32_t s_4 = vgetq_lane_f32(s_32x4_1, 0);
            float32_t s_5 = vgetq_lane_f32(s_32x4_1, 1);
            float32_t s_6 = vgetq_lane_f32(s_32x4_1, 2);
            float32_t s_7 = vgetq_lane_f32(s_32x4_1, 3);
            
            float32x4_t t_32x4 = vsubq_f32(y_src_32x4, y_min_32x4);
            
            float32_t t_0 = vgetq_lane_f32(t_32x4, 0);
            float32_t t_1 = vgetq_lane_f32(t_32x4, 1);
            float32_t t_2 = vgetq_lane_f32(t_32x4, 2);
            float32_t t_3 = vgetq_lane_f32(t_32x4, 3);
            
            // the first time
            vx_bool defined_tl_0 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 0), (vx_int32)vgetq_lane_f32(y_min_32x4, 0), &tl, borders);
            vx_bool defined_tr_0 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 0)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 0), &tr, borders);
            vx_bool defined_bl_0 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 0), (vx_int32)vgetq_lane_f32(y_min_32x4, 0)+1, &bl, borders);
            vx_bool defined_br_0 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 0)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 0)+1, &br, borders);
            vx_bool defined_0 = defined_tl_0 & defined_tr_0 & defined_bl_0 & defined_br_0;
            if (defined_0 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_0 | defined_tr_0 | defined_bl_0 | defined_br_0;
                if (defined_any)
                {
                    if ((defined_tl_0 == vx_false_e || defined_tr_0 == vx_false_e) && fabs(t_0 - 1.0) <= 0.001)
                        defined_tl_0 = defined_tr_0 = vx_true_e;
                    else if ((defined_bl_0 == vx_false_e || defined_br_0 == vx_false_e) && fabs(t_0 - 0.0) <= 0.001)
                        defined_bl_0 = defined_br_0 = vx_true_e;
                    if ((defined_tl_0 == vx_false_e || defined_bl_0 == vx_false_e) && fabs(s_0 - 1.0) <= 0.001)
                        defined_tl_0 = defined_bl_0 = vx_true_e;
                    else if ((defined_tr_0 == vx_false_e || defined_br_0 == vx_false_e) && fabs(s_0 - 0.0) <= 0.001)
                        defined_tr_0 = defined_br_0 = vx_true_e;
                    defined_0 = defined_tl_0 & defined_tr_0 & defined_bl_0 & defined_br_0;
                }
            }
            if (defined_0 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_0) * (1 - t_0) * tl +
                        (    s_0) * (1 - t_0) * tr +
                        (1 - s_0) * (    t_0) * bl +
                        (    s_0) * (    t_0) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst)
                    *dst = ref_8u;
            }
            
            // the second time
            vx_bool defined_tl_1 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 1), (vx_int32)vgetq_lane_f32(y_min_32x4, 1), &tl, borders);
            vx_bool defined_tr_1 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 1)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 1), &tr, borders);
            vx_bool defined_bl_1 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 1), (vx_int32)vgetq_lane_f32(y_min_32x4, 1)+1, &bl, borders);
            vx_bool defined_br_1 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 1)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 1)+1, &br, borders);
            vx_bool defined_1 = defined_tl_1 & defined_tr_1 & defined_bl_1 & defined_br_1;
            if (defined_1 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_1 | defined_tr_1 | defined_bl_1 | defined_br_1;
                if (defined_any)
                {
                    if ((defined_tl_1 == vx_false_e || defined_tr_1 == vx_false_e) && fabs(t_1 - 1.0) <= 0.001)
                        defined_tl_1 = defined_tr_1 = vx_true_e;
                    else if ((defined_bl_1 == vx_false_e || defined_br_1 == vx_false_e) && fabs(t_1 - 0.0) <= 0.001)
                        defined_bl_1 = defined_br_1 = vx_true_e;
                    if ((defined_tl_1 == vx_false_e || defined_bl_1 == vx_false_e) && fabs(s_1 - 1.0) <= 0.001)
                        defined_tl_1 = defined_bl_1 = vx_true_e;
                    else if ((defined_tr_1 == vx_false_e || defined_br_1 == vx_false_e) && fabs(s_1 - 0.0) <= 0.001)
                        defined_tr_1 = defined_br_1 = vx_true_e;
                    defined_1 = defined_tl_1 & defined_tr_1 & defined_bl_1 & defined_br_1;
                }
            }
            if (defined_1 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_1) * (1 - t_1) * tl +
                        (    s_1) * (1 - t_1) * tr +
                        (1 - s_1) * (    t_1) * bl +
                        (    s_1) * (    t_1) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst+1)
                    *(dst+1) = ref_8u;
            }
            
            // the third time
            vx_bool defined_tl_2 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 2), (vx_int32)vgetq_lane_f32(y_min_32x4, 2), &tl, borders);
            vx_bool defined_tr_2 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 2)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 2), &tr, borders);
            vx_bool defined_bl_2 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 2), (vx_int32)vgetq_lane_f32(y_min_32x4, 2)+1, &bl, borders);
            vx_bool defined_br_2 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 2)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 2)+1, &br, borders);
            vx_bool defined_2 = defined_tl_2 & defined_tr_2 & defined_bl_2 & defined_br_2;
            if (defined_2 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_2 | defined_tr_2 | defined_bl_2 | defined_br_2;
                if (defined_any)
                {
                    if ((defined_tl_2 == vx_false_e || defined_tr_2 == vx_false_e) && fabs(t_2 - 1.0) <= 0.001)
                        defined_tl_2 = defined_tr_2 = vx_true_e;
                    else if ((defined_bl_2 == vx_false_e || defined_br_2 == vx_false_e) && fabs(t_2 - 0.0) <= 0.001)
                        defined_bl_2 = defined_br_2 = vx_true_e;
                    if ((defined_tl_2 == vx_false_e || defined_bl_2 == vx_false_e) && fabs(s_2 - 1.0) <= 0.001)
                        defined_tl_2 = defined_bl_2 = vx_true_e;
                    else if ((defined_tr_2 == vx_false_e || defined_br_2 == vx_false_e) && fabs(s_2 - 0.0) <= 0.001)
                        defined_tr_2 = defined_br_2 = vx_true_e;
                    defined_2 = defined_tl_2 & defined_tr_2 & defined_bl_2 & defined_br_2;
                }
            }
            if (defined_2 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_2) * (1 - t_2) * tl +
                        (    s_2) * (1 - t_2) * tr +
                        (1 - s_2) * (    t_2) * bl +
                        (    s_2) * (    t_2) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst+2)
                    *(dst+2) = ref_8u;
            }
            
            // the fourth time
            vx_bool defined_tl_3 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 3), (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tl, borders);
            vx_bool defined_tr_3 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 3)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tr, borders);
            vx_bool defined_bl_3 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 3), (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &bl, borders);
            vx_bool defined_br_3 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4, 3)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &br, borders);
            vx_bool defined_3 = defined_tl_3 & defined_tr_3 & defined_bl_3 & defined_br_3;
            if (defined_3 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_3 | defined_tr_3 | defined_bl_3 | defined_br_3;
                if (defined_any)
                {
                    if ((defined_tl_3 == vx_false_e || defined_tr_3 == vx_false_e) && fabs(t_3 - 1.0) <= 0.001)
                        defined_tl_3 = defined_tr_3 = vx_true_e;
                    else if ((defined_bl_3 == vx_false_e || defined_br_3 == vx_false_e) && fabs(t_3 - 0.0) <= 0.001)
                        defined_bl_3 = defined_br_3 = vx_true_e;
                    if ((defined_tl_3 == vx_false_e || defined_bl_3 == vx_false_e) && fabs(s_3 - 1.0) <= 0.001)
                        defined_tl_3 = defined_bl_3 = vx_true_e;
                    else if ((defined_tr_3 == vx_false_e || defined_br_3 == vx_false_e) && fabs(s_3 - 0.0) <= 0.001)
                        defined_tr_3 = defined_br_3 = vx_true_e;
                    defined_3 = defined_tl_3 & defined_tr_3 & defined_bl_3 & defined_br_3;
                }
            }
            if (defined_3 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_3) * (1 - t_3) * tl +
                        (    s_3) * (1 - t_3) * tr +
                        (1 - s_3) * (    t_3) * bl +
                        (    s_3) * (    t_3) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst+3)
                    *(dst+3) = ref_8u;
            }
            
            // the fifth time
            vx_bool defined_tl_4 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 0), (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tl, borders);
            vx_bool defined_tr_4 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 0)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tr, borders);
            vx_bool defined_bl_4 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 0), (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &bl, borders);
            vx_bool defined_br_4 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 0)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &br, borders);
            vx_bool defined_4 = defined_tl_4 & defined_tr_4 & defined_bl_4 & defined_br_4;
            if (defined_4 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_4 | defined_tr_4 | defined_bl_4 | defined_br_4;
                if (defined_any)
                {
                    if ((defined_tl_4 == vx_false_e || defined_tr_4 == vx_false_e) && fabs(t_3 - 1.0) <= 0.001)
                        defined_tl_4 = defined_tr_4 = vx_true_e;
                    else if ((defined_bl_4 == vx_false_e || defined_br_4 == vx_false_e) && fabs(t_3 - 0.0) <= 0.001)
                        defined_bl_4 = defined_br_4 = vx_true_e;
                    if ((defined_tl_4 == vx_false_e || defined_bl_4 == vx_false_e) && fabs(s_4 - 1.0) <= 0.001)
                        defined_tl_4 = defined_bl_4 = vx_true_e;
                    else if ((defined_tr_4 == vx_false_e || defined_br_4 == vx_false_e) && fabs(s_4 - 0.0) <= 0.001)
                        defined_tr_4 = defined_br_4 = vx_true_e;
                    defined_4 = defined_tl_4 & defined_tr_4 & defined_bl_4 & defined_br_4;
                }
            }
            if (defined_4 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_4) * (1 - t_3) * tl +
                        (    s_4) * (1 - t_3) * tr +
                        (1 - s_4) * (    t_3) * bl +
                        (    s_4) * (    t_3) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst+4)
                    *(dst+4) = ref_8u;
            }
            
            // the sixth time
            vx_bool defined_tl_5 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 1), (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tl, borders);
            vx_bool defined_tr_5 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 1)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tr, borders);
            vx_bool defined_bl_5 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 1), (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &bl, borders);
            vx_bool defined_br_5 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 1)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &br, borders);
            vx_bool defined_5 = defined_tl_5 & defined_tr_5 & defined_bl_5 & defined_br_5;
            if (defined_5 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_5 | defined_tr_5 | defined_bl_5 | defined_br_5;
                if (defined_any)
                {
                    if ((defined_tl_5 == vx_false_e || defined_tr_5 == vx_false_e) && fabs(t_3 - 1.0) <= 0.001)
                        defined_tl_5 = defined_tr_5 = vx_true_e;
                    else if ((defined_bl_5 == vx_false_e || defined_br_5 == vx_false_e) && fabs(t_3 - 0.0) <= 0.001)
                        defined_bl_5 = defined_br_5 = vx_true_e;
                    if ((defined_tl_5 == vx_false_e || defined_bl_5 == vx_false_e) && fabs(s_5 - 1.0) <= 0.001)
                        defined_tl_5 = defined_bl_5 = vx_true_e;
                    else if ((defined_tr_5 == vx_false_e || defined_br_5 == vx_false_e) && fabs(s_5 - 0.0) <= 0.001)
                        defined_tr_5 = defined_br_5 = vx_true_e;
                    defined_5 = defined_tl_5 & defined_tr_5 & defined_bl_5 & defined_br_5;
                }
            }
            if (defined_5 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_5) * (1 - t_3) * tl +
                        (    s_5) * (1 - t_3) * tr +
                        (1 - s_5) * (    t_3) * bl +
                        (    s_5) * (    t_3) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst+5)
                    *(dst+5) = ref_8u;
            }
                
            // the seventh time
            vx_bool defined_tl_6 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 2), (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tl, borders);
            vx_bool defined_tr_6 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 2)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tr, borders);
            vx_bool defined_bl_6 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 2), (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &bl, borders);
            vx_bool defined_br_6 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 2)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &br, borders);
            vx_bool defined_6 = defined_tl_6 & defined_tr_6 & defined_bl_6 & defined_br_6;
            if (defined_6 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_6 | defined_tr_6 | defined_bl_6 | defined_br_6;
                if (defined_any)
                {
                    if ((defined_tl_6 == vx_false_e || defined_tr_6 == vx_false_e) && fabs(t_3 - 1.0) <= 0.001)
                        defined_tl_6 = defined_tr_6 = vx_true_e;
                    else if ((defined_bl_6 == vx_false_e || defined_br_6 == vx_false_e) && fabs(t_3 - 0.0) <= 0.001)
                        defined_bl_6 = defined_br_6 = vx_true_e;
                    if ((defined_tl_6 == vx_false_e || defined_bl_6 == vx_false_e) && fabs(s_6 - 1.0) <= 0.001)
                        defined_tl_6 = defined_bl_6 = vx_true_e;
                    else if ((defined_tr_6 == vx_false_e || defined_br_6 == vx_false_e) && fabs(s_6 - 0.0) <= 0.001)
                        defined_tr_6 = defined_br_6 = vx_true_e;
                    defined_6 = defined_tl_6 & defined_tr_6 & defined_bl_6 & defined_br_6;
                }
            }
            if (defined_6 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_6) * (1 - t_3) * tl +
                        (    s_6) * (1 - t_3) * tr +
                        (1 - s_6) * (    t_3) * bl +
                        (    s_6) * (    t_3) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst+6)
                    *(dst+6) = ref_8u;
            }
            
            // the eighth time
            vx_bool defined_tl_7 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 3), (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tl, borders);
            vx_bool defined_tr_7 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 3)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3), &tr, borders);
            vx_bool defined_bl_7 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 3), (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &bl, borders);
            vx_bool defined_br_7 = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, (vx_int32)vgetq_lane_f32(x_min_32x4_1, 3)+1, (vx_int32)vgetq_lane_f32(y_min_32x4, 3)+1, &br, borders);
            vx_bool defined_7 = defined_tl_7 & defined_tr_7 & defined_bl_7 & defined_br_7;
            if (defined_7 == vx_false_e)
            {
                vx_bool defined_any = defined_tl_7 | defined_tr_7 | defined_bl_7 | defined_br_7;
                if (defined_any)
                {
                    if ((defined_tl_7 == vx_false_e || defined_tr_7 == vx_false_e) && fabs(t_3 - 1.0) <= 0.001)
                        defined_tl_7 = defined_tr_7 = vx_true_e;
                    else if ((defined_bl_7 == vx_false_e || defined_br_7 == vx_false_e) && fabs(t_3 - 0.0) <= 0.001)
                        defined_bl_7 = defined_br_7 = vx_true_e;
                    if ((defined_tl_7 == vx_false_e || defined_bl_7 == vx_false_e) && fabs(s_7 - 1.0) <= 0.001)
                        defined_tl_7 = defined_bl_7 = vx_true_e;
                    else if ((defined_tr_7 == vx_false_e || defined_br_7 == vx_false_e) && fabs(s_7 - 0.0) <= 0.001)
                        defined_tr_7 = defined_br_7 = vx_true_e;
                    defined_7 = defined_tl_7 & defined_tr_7 & defined_bl_7 & defined_br_7;
                }
            }
            if (defined_7 == vx_true_e)
            {
                vx_float32 ref =
                        (1 - s_7) * (1 - t_3) * tl +
                        (    s_7) * (1 - t_3) * tr +
                        (1 - s_7) * (    t_3) * bl +
                        (    s_7) * (    t_3) * br;
                vx_uint8 ref_8u;
                if (ref > 255)
                    ref_8u = 255;
                else
                    ref_8u = (vx_uint8)ref;
                if (dst+7)
                    *(dst+7) = ref_8u;
            }
        }
    }
}

void ScaleImage_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];
    vx_enum *type = (vx_enum*)parameters[2];
    if (*type == VX_INTERPOLATION_BILINEAR)
    {
        vxBilinearScaling_fast(in, out);
    }
    else if (*type == VX_INTERPOLATION_AREA)
    {
        vxNearestScaling_fast(in, out);
    }
    else if (*type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
    {
        vxNearestScaling_fast(in, out);
    }
}


#define NEAREST_SCALING(low_y, low_x, high_y, high_x, src_image_tile_x, dst_image_tile_x) \
    for (y2 = low_y; y2 < high_y; y2++)\
    {\
        for (x2 = low_x; x2 < high_x; x2++)\
        {\
            if (VX_DF_IMAGE_U8 == format)\
            {\
                vx_uint8 v = 0;\
                vx_uint8 *dst = (vx_uint8 *)dst_image->base[0] + y2 * dst_image->addr[0].stride_y + x2 * dst_image->addr[0].stride_x;\
                vx_float32 x_src = ((vx_float32)x2 + 0.5f)*wr - 0.5f;\
                vx_float32 y_src = ((vx_float32)y2 + 0.5f)*hr - 0.5f;\
                vx_float32 x_min = floorf(x_src);\
                vx_float32 y_min = floorf(y_src);\
                x1 = (vx_int32)x_min;\
                y1 = (vx_int32)y_min;\
                if (x_src - x_min >= 0.5f)\
                    x1++;\
                if (y_src - y_min >= 0.5f)\
                    y1++;\
                if (dst && vx_true_e == read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, x1, y1, &v, borders))\
                    *dst = v;\
            }\
            else\
            {\
                vx_int16 v = 0;\
                vx_int16 *dst = (vx_int16 *)dst_image->base[0] + y2 * dst_image->addr[0].stride_y / 2+ x2 * dst_image->addr[0].stride_x /2;\
                vx_float32 x_src = ((vx_float32)x2 + 0.5f)*wr - 0.5f;\
                vx_float32 y_src = ((vx_float32)y2 + 0.5f)*hr - 0.5f;\
                vx_float32 x_min = floorf(x_src);\
                vx_float32 y_min = floorf(y_src);\
                x1 = (vx_int32)x_min;\
                y1 = (vx_int32)y_min;\
                if (x_src - x_min >= 0.5f)\
                    x1++;\
                if (y_src - y_min >= 0.5f)\
                    y1++;\
                if (dst && vx_true_e == read_pixel_16s((vx_int16 *)src_image->base[0], src_image->addr, h1, w1, x1, y1, &v, borders))\
                    *dst = v;\
            }\
        }\
    }\

static void vxNearestScaling(vx_tile_ex_t *src_image, vx_tile_ex_t *dst_image, vx_uint32 ty, vx_uint32 tx)
{
    vx_int32 x1,y1,x2,y2;
    vx_rectangle_t src_rect, dst_rect;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
    vx_float32 wr, hr;
    vx_df_image format = 0;

    w1 = src_image->image.width;
    h1 = src_image->image.height;
    format = src_image->image.format;
    w2 = dst_image->image.width;
    h2 = dst_image->image.height;
    
    src_rect.start_x = src_rect.start_y = 0;
    src_rect.end_x = w1;
    src_rect.end_y = h1;

    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    wr = (vx_float32)w1/(vx_float32)w2;
    hr = (vx_float32)h1/(vx_float32)h2;

    vx_border_t borders = src_image->border;
    
    if (src_image->is_U1 == 0)
    {
        if (ty == 0 && tx == 0)
        {
            NEAREST_SCALING(0, 0, vxTileHeight(dst_image, 0), vxTileWidth(dst_image, 0), src_image->tile_x, dst_image->tile_x)
        }
        else
        {
            NEAREST_SCALING(0, tx, ty, vxTileWidth(dst_image, 0), src_image->tile_x, dst_image->tile_x)
            NEAREST_SCALING(ty, 0, vxTileHeight(dst_image, 0), vxTileWidth(dst_image, 0), 0, 0)
        }
    }
    else
    {
        void *src_base = src_image->base[0];                                                           
        void *dst_base = dst_image->base[0];
        w1 = src_image->rect.end_x - src_image->rect.start_x + src_image->rect.start_x % 8;
        h1 = src_image->rect.end_y - src_image->rect.start_y;
        for (y2 = 0; y2 < (vx_int32)h2; y2++)
        {
            for (x2 = 0; x2 < (vx_int32)w2; x2++)
            {
                vx_uint8 v = 0;
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x2, y2, dst_image->addr);
                vx_float32 x_src = ((vx_float32)x2 + 0.5f)*wr - 0.5f;
                vx_float32 y_src = ((vx_float32)y2 + 0.5f)*hr - 0.5f;

                // Switch to coordinates in the mapped image patch
                x_src = x_src - src_image->rect.start_x + src_image->rect.start_x % 8;
                y_src = y_src - src_image->rect.start_y;
                vx_float32 x_min = floorf(x_src);
                vx_float32 y_min = floorf(y_src);
                x1 = (vx_int32)x_min;
                y1 = (vx_int32)y_min;

                if (x_src - x_min >= 0.5f)
                    x1++;
                if (y_src - y_min >= 0.5f)
                    y1++;

                if (dst && vx_true_e == read_pixel_1u(src_base, src_image->addr, h1, w1, x1, y1, &v, src_image->rect.start_x % 8, borders))
                    *dst = (*dst & ~(1 << (x2 % 8))) | (v << (x2 % 8));
            }
        }
    }
}

#define BILINEAR_SCALING(low_y, low_x, high_y, high_x, src_image_tile_x, dst_image_tile_x) \
    for (y2 = low_y; y2 < high_y; y2++)\
    {\
        for (x2 = low_x; x2 < high_x; x2++)\
        {\
            vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;\
            vx_uint8 *dst = (vx_uint8 *)dst_image->base[0] + y2 * dst_image->addr[0].stride_y + x2 * dst_image->addr[0].stride_x;\
            vx_float32 x_src = ((vx_float32)x2+0.5f)*wr - 0.5f;\
            vx_float32 y_src = ((vx_float32)y2+0.5f)*hr - 0.5f;\
            vx_float32 x_min = floorf(x_src);\
            vx_float32 y_min = floorf(y_src);\
            vx_int32 x1 = (vx_int32)x_min;\
            vx_int32 y1 = (vx_int32)y_min;\
            vx_float32 s = x_src - x_min;\
            vx_float32 t = y_src - y_min;\
            vx_bool defined_tl = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, x1 + 0, y1 + 0, &tl, borders);\
            vx_bool defined_tr = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, x1 + 1, y1 + 0, &tr, borders);\
            vx_bool defined_bl = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, x1 + 0, y1 + 1, &bl, borders);\
            vx_bool defined_br = read_pixel((vx_uint8 *)src_image->base[0], src_image->addr, h1, w1, x1 + 1, y1 + 1, &br, borders);\
            vx_bool defined = defined_tl & defined_tr & defined_bl & defined_br;\
            if (defined == vx_false_e)\
            {\
                vx_bool defined_any = defined_tl | defined_tr | defined_bl | defined_br;\
                if (defined_any)\
                {\
                    if ((defined_tl == vx_false_e || defined_tr == vx_false_e) && fabs(t - 1.0) <= 0.001)\
                        defined_tl = defined_tr = vx_true_e;\
                    else if ((defined_bl == vx_false_e || defined_br == vx_false_e) && fabs(t - 0.0) <= 0.001)\
                        defined_bl = defined_br = vx_true_e;\
                    if ((defined_tl == vx_false_e || defined_bl == vx_false_e) && fabs(s - 1.0) <= 0.001)\
                        defined_tl = defined_bl = vx_true_e;\
                    else if ((defined_tr == vx_false_e || defined_br == vx_false_e) && fabs(s - 0.0) <= 0.001)\
                        defined_tr = defined_br = vx_true_e;\
                    defined = defined_tl & defined_tr & defined_bl & defined_br;\
                }\
            }\
            if (defined == vx_true_e)\
            {\
                vx_float32 ref =\
                        (1 - s) * (1 - t) * tl +\
                        (    s) * (1 - t) * tr +\
                        (1 - s) * (    t) * bl +\
                        (    s) * (    t) * br;\
                vx_uint8 ref_8u;\
                if (ref > 255)\
                    ref_8u = 255;\
                else\
                    ref_8u = (vx_uint8)ref;\
                if (dst)\
                    *dst = ref_8u;\
            }\
        }\
    }\


static void vxBilinearScaling(vx_tile_ex_t *src_image, vx_tile_ex_t *dst_image, vx_uint32 ty, vx_uint32 tx)
{
    vx_int32 x2,y2;
    vx_rectangle_t src_rect, dst_rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
    vx_float32 wr, hr;

    w1 = src_image->image.width;
    h1 = src_image->image.height;
    w2 = dst_image->image.width;
    h2 = dst_image->image.height;

    src_rect.start_x = src_rect.start_y = 0;
    src_rect.end_x = w1;
    src_rect.end_y = h1;

    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    wr = (vx_float32)w1/(vx_float32)w2;
    hr = (vx_float32)h1/(vx_float32)h2;

    vx_border_t borders = src_image->border;

    if (src_image->is_U1 == 0)
    {
        if (ty == 0 && tx == 0)
        {
            BILINEAR_SCALING(0, 0, vxTileHeight(dst_image, 0), vxTileWidth(dst_image, 0), src_image->tile_x, dst_image->tile_x)
        }
        else
        {
            BILINEAR_SCALING(0, tx, ty, vxTileWidth(dst_image, 0), src_image->tile_x, dst_image->tile_x)
            BILINEAR_SCALING(ty, 0, vxTileHeight(dst_image, 0), vxTileWidth(dst_image, 0), 0, 0)
        }
    }
    else
    {
        void *src_base = src_image->base[0];                                                           
        void *dst_base = dst_image->base[0];
        w1 = src_image->rect.end_x - src_image->rect.start_x + src_image->rect.start_x % 8;
        h1 = src_image->rect.end_y - src_image->rect.start_y;
        for (y2 = 0; y2 < (vx_int32)h2; y2++)
        {
            for (x2 = 0; x2 < (vx_int32)w2; x2++)
            {
                vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x2, y2, dst_image->addr);
                vx_float32 x_src = ((vx_float32)x2+0.5f)*wr - 0.5f;
                vx_float32 y_src = ((vx_float32)y2+0.5f)*hr - 0.5f;

                // Switch to coordinates in the mapped image patch
                x_src = x_src - src_image->rect.start_x + src_image->rect.start_x % 8;
                y_src = y_src - src_image->rect.start_y;
                vx_float32 x_min = floorf(x_src);
                vx_float32 y_min = floorf(y_src);
                vx_int32 x1 = (vx_int32)x_min;
                vx_int32 y1 = (vx_int32)y_min;
                vx_float32 s = x_src - x_min;
                vx_float32 t = y_src - y_min;

                vx_bool defined_tl, defined_tr, defined_bl, defined_br;

                defined_tl = read_pixel_1u(src_base, src_image->addr, h1, w1, x1 + 0, y1 + 0, &tl, src_image->rect.start_x % 8, borders);
                defined_tr = read_pixel_1u(src_base, src_image->addr, h1, w1, x1 + 1, y1 + 0, &tr, src_image->rect.start_x % 8, borders);
                defined_bl = read_pixel_1u(src_base, src_image->addr, h1, w1, x1 + 0, y1 + 1, &bl, src_image->rect.start_x % 8, borders);
                defined_br = read_pixel_1u(src_base, src_image->addr, h1, w1, x1 + 1, y1 + 1, &br, src_image->rect.start_x % 8, borders);

                vx_bool defined = defined_tl & defined_tr & defined_bl & defined_br;
                if (defined == vx_false_e)
                {
                    vx_bool defined_any = defined_tl | defined_tr | defined_bl | defined_br;
                    if (defined_any)
                    {
                        if ((defined_tl == vx_false_e || defined_tr == vx_false_e) && fabs(t - 1.0) <= 0.001)
                            defined_tl = defined_tr = vx_true_e;
                        else if ((defined_bl == vx_false_e || defined_br == vx_false_e) && fabs(t - 0.0) <= 0.001)
                            defined_bl = defined_br = vx_true_e;
                        if ((defined_tl == vx_false_e || defined_bl == vx_false_e) && fabs(s - 1.0) <= 0.001)
                            defined_tl = defined_bl = vx_true_e;
                        else if ((defined_tr == vx_false_e || defined_br == vx_false_e) && fabs(s - 0.0) <= 0.001)
                            defined_tr = defined_br = vx_true_e;
                        defined = defined_tl & defined_tr & defined_bl & defined_br;
                    }
                }

                if (defined == vx_true_e)
                {
                    vx_float32 ref =
                            (1 - s) * (1 - t) * tl +
                            (    s) * (1 - t) * tr +
                            (1 - s) * (    t) * bl +
                            (    s) * (    t) * br;
                    vx_uint8 ref_8u;
                    ref = ref + 0.5f;

                    if (ref > 1)
                        ref_8u = 1;
                    else
                        ref_8u = (vx_uint8)ref;

                    if (dst)
                        *dst = (*dst & ~(1 << (x2 % 8))) | (ref_8u << (x2 % 8));
                }
            }
        }
    }
}

void ScaleImage_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_tile_ex_t *out = (vx_tile_ex_t *)parameters[1];
    vx_enum *type = (vx_enum*)parameters[2];
    vx_uint32 ty = out->tile_y;
    vx_uint32 tx = out->tile_x;
    if (*type == VX_INTERPOLATION_BILINEAR)
    {
        vxBilinearScaling(in, out, ty, tx);
    }
    else if (*type == VX_INTERPOLATION_AREA)
    {
        vxNearestScaling(in, out, ty, tx);
    }
    else if (*type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
    {
        vxNearestScaling(in, out, ty, tx);
    }
}


