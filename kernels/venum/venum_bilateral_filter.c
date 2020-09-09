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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <venum.h>
#include <arm_neon.h>

#define  COLOR_WEIGHT_SIZE_PER_CHANNEL      256

static void getMinMax(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                      vx_int16 *max_value, vx_int16 *min_value)
{
    vx_int16 maxVal = INT16_MIN;
    vx_int16 minVal = INT16_MAX;
    if (num_of_dims == 2)
    {
        for (vx_uint32 y = 0; y < dims[1]; y++)
        {
            for (vx_uint32 x = 0; x < dims[0]; x++)
            {
                vx_uint32 offset = y * src_strides[1] + x * src_strides[0];
                vx_int16 val = *(vx_int16 *)((vx_int8 *)src + offset);
                if (val > maxVal)
                {
                    maxVal = val;
                }
                if (val < minVal)
                {
                    minVal = val;
                }
            }
        }
        *max_value = maxVal;
        *min_value = minVal;
    }
    else if (num_of_dims == 3)
    {
        for (vx_uint32 y = 0; y < dims[2]; y++)
        {
            for (vx_uint32 x = 0; x < dims[1]; x++)
            {
                for (vx_uint32 z = 0; z < dims[0]; z++)
                {
                    vx_uint32 offset = y * src_strides[2] + x * src_strides[1] + z * src_strides[0];
                    vx_int16 val = *(vx_int16 *)((vx_int8 *)src + offset);
                    if (val > maxVal)
                    {
                        maxVal = val;
                    }
                    if (val < minVal)
                    {
                        minVal = val;
                    }
                }
            }
        }
        *max_value = maxVal;
        *min_value = minVal;
    }
}

static void releaseRes(void *pData)
{
    if (NULL != pData)
    {
        free(pData);
    }
    return;
}

static vx_status calcColorWeight(vx_uint8 cn, vx_float64 gauss_color_coeff, vx_float32 **color_weight)
{
    vx_float32 *tmp_weight = (vx_float32 *)malloc(cn * COLOR_WEIGHT_SIZE_PER_CHANNEL * sizeof(vx_float32));
    if (NULL == tmp_weight)
    {
        return VX_ERROR_NO_MEMORY;
    }

    for (vx_int32 i = 0; i < (cn * COLOR_WEIGHT_SIZE_PER_CHANNEL); i++)
    {
        tmp_weight[i] = (vx_float32)exp(i * i * gauss_color_coeff);
    }

    *color_weight = tmp_weight;

    return VX_SUCCESS;
}

static vx_status calcSpaceWeight(vx_int32 diameter, vx_float64 gauss_space_coeff, vx_float32 **space_weight)
{
    vx_int32 radius = diameter / 2;
    vx_float32 *tmp_weight = (vx_float32 *)malloc(diameter * diameter * sizeof(vx_float32));
    if (NULL == tmp_weight)
    {
        return VX_ERROR_NO_MEMORY;
    }

    for (vx_int32 i = -radius; i <= radius; i++)
    {
        vx_int32 j = -radius;
        for (; j <= radius; j++)
        {
            vx_float64 r = sqrt((vx_float64)i * i + (vx_float64)j * j);
            if (r > radius)
            {
                continue;
            }
            tmp_weight[(i + radius) * diameter + (j + radius)] = (vx_float32)exp(r * r * gauss_space_coeff);
        }
    }

    *space_weight = tmp_weight;

    return VX_SUCCESS;
}


static vx_status bilateralFilter_8u(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                                    vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                                    void* dst, vx_size* dst_strides, vx_border_t *bordermode)
{
    vx_status status = VX_SUCCESS;
    vx_int32 y = 0, x = 0;
    vx_int32 radius_y, radius_x;
    vx_int32 radius = diameter/2;
    vx_enum border_mode = bordermode->mode;
    vx_int32 low_x, low_y, high_x, high_y;
    vx_float32 *color_weight = NULL;
    vx_float32 *space_weight = NULL;
    vx_uint8 cn = num_of_dims == 2 ? 1 : 3;

    vx_float64 gauss_color_coeff = -0.5/(sigma_color*sigma_color);
    vx_float64 gauss_space_coeff = -0.5/(sigma_space*sigma_space);

    status = calcColorWeight(cn, gauss_color_coeff, &color_weight);
    status |= calcSpaceWeight(diameter, gauss_space_coeff, &space_weight);
    if (status != VX_SUCCESS)
    {
        releaseRes(color_weight);
        releaseRes(space_weight);
    }

    // Calculate radius
    vx_int32 rx = 0, ry = 0;
    vx_int32 *Radius_y = (vx_int32 *)calloc(diameter * diameter, sizeof(vx_int32));
    vx_int32 *Radius_x = (vx_int32 *)calloc(diameter * diameter, sizeof(vx_int32));
    for (radius_y = -radius; radius_y <= radius; radius_y++)
    {
        for (radius_x = -radius; radius_x <= radius; radius_x++)
        {
            vx_float64 r = sqrt((vx_float64)radius_y * radius_y + (vx_float64)radius_x * radius_x);
            if (r > radius)
            {
                continue;
            }
            Radius_y[ry++] = radius_y;
            Radius_x[rx++] = radius_x;
        }
    }
    vx_int32 maxk = rx;

    if (bordermode->mode == VX_BORDER_UNDEFINED)
    {
        low_x = radius;
        high_x = (dims[num_of_dims - 2] >= radius) ? dims[num_of_dims - 2] - radius : 0;
        low_y = radius;
        high_y = (dims[num_of_dims - 1] >= radius) ? dims[num_of_dims - 1] - radius : 0;
    }
    else
    {
        low_x = 0;
        high_x = dims[num_of_dims - 2];
        low_y = 0;
        high_y = dims[num_of_dims - 1];
    }

    if (num_of_dims == 2)
    {
        for (y = low_y ; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x += 4)
            {
                float32x4_t v_sum = vdupq_n_f32(0.0f), v_wsum = vdupq_n_f32(0.0f);
                float32x4_t value;
                value = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[1] + x * src_strides[0]), value, 0);
                value = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[1] + (x + 1) * src_strides[0]), value, 1);
                value = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[1] + (x + 2) * src_strides[0]), value, 2);
                value = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[1] + (x + 3) * src_strides[0]), value, 3);
                //kernel filter
                for (vx_int32 k = 0; k < maxk; k++)
                {
                    vx_int32 neighbor_x = x + Radius_x[k];
                    vx_int32 neighbor_x1 = (x + 1) + Radius_x[k];
                    vx_int32 neighbor_x2 = (x + 2) + Radius_x[k];
                    vx_int32 neighbor_x3 = (x + 3) + Radius_x[k];

                    vx_int32 neighbor_y = y + Radius_y[k];
                    vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[0] - 1) ? (dims[0] - 1) : neighbor_x);
                    vx_int32 tmpx1 = neighbor_x1 < 0 ? 0 : (neighbor_x1 >(dims[0] - 1) ? (dims[0] - 1) : neighbor_x1);
                    vx_int32 tmpx2 = neighbor_x2 < 0 ? 0 : (neighbor_x2 >(dims[0] - 1) ? (dims[0] - 1) : neighbor_x2);
                    vx_int32 tmpx3 = neighbor_x3 < 0 ? 0 : (neighbor_x3 >(dims[0] - 1) ? (dims[0] - 1) : neighbor_x3);

                    vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[1] - 1) ? (dims[1] - 1) : neighbor_y);

                    float32x4_t neighborVal;
                    neighborVal = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[1] + tmpx * src_strides[0]), neighborVal, 0);
                    neighborVal = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[1] + tmpx1 * src_strides[0]), neighborVal, 1);
                    neighborVal = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[1] + tmpx2 * src_strides[0]), neighborVal, 2);
                    neighborVal = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[1] + tmpx3 * src_strides[0]), neighborVal, 3);

                    if (neighbor_x < 0 || neighbor_y < 0 || neighbor_x >= dims[0] || neighbor_y >= dims[1])
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.U8, neighborVal, 0);

                    if (neighbor_x1 < 0 || neighbor_y < 0 || neighbor_x1 >= dims[0] || neighbor_y >= dims[1])
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.U8, neighborVal, 1);

                    if (neighbor_x2 < 0 || neighbor_y < 0 || neighbor_x2 >= dims[0] || neighbor_y >= dims[1])
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.U8, neighborVal, 2);

                    if (neighbor_x3 < 0 || neighbor_y < 0 || neighbor_x3 >= dims[0] || neighbor_y >= dims[1])
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.U8, neighborVal, 3);

                    vx_int32 weight_k = (Radius_y[k] + radius) * diameter + (Radius_x[k] + radius);
                    float32x4_t space_weight_val = vdupq_n_f32(space_weight[weight_k]);
                    float32x4_t v_abs_val = vabdq_f32(neighborVal, value);
                    vx_int32 abs_val0 = (vx_int32)vgetq_lane_f32(v_abs_val, 0);
                    vx_int32 abs_val1 = (vx_int32)vgetq_lane_f32(v_abs_val, 1);
                    vx_int32 abs_val2 = (vx_int32)vgetq_lane_f32(v_abs_val, 2);
                    vx_int32 abs_val3 = (vx_int32)vgetq_lane_f32(v_abs_val, 3);
                    float32x4_t color_weight_val;
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val0], color_weight_val, 0);
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val1], color_weight_val, 1);
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val2], color_weight_val, 2);
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val3], color_weight_val, 3);
                    float32x4_t w = vmulq_f32(space_weight_val, color_weight_val);

                    v_sum = vaddq_f32(v_sum, vmulq_f32(neighborVal, w));
                    v_wsum = vaddq_f32(v_wsum, w);
                }

                vx_float32 sum[4], wsum[4];
                sum[0] = vgetq_lane_f32(v_sum, 0);
                sum[1] = vgetq_lane_f32(v_sum, 1);
                sum[2] = vgetq_lane_f32(v_sum, 2);
                sum[3] = vgetq_lane_f32(v_sum, 3);
                wsum[0] = vgetq_lane_f32(v_wsum, 0);
                wsum[1] = vgetq_lane_f32(v_wsum, 1);
                wsum[2] = vgetq_lane_f32(v_wsum, 2);
                wsum[3] = vgetq_lane_f32(v_wsum, 3);

                *((vx_uint8 *)dst + y * src_strides[1] + x * src_strides[0]) = (vx_uint8)roundf(sum[0] / wsum[0]);
                *((vx_uint8 *)dst + y * src_strides[1] + (x + 1) * src_strides[0]) = (vx_uint8)roundf(sum[1] / wsum[1]);
                *((vx_uint8 *)dst + y * src_strides[1] + (x + 2) * src_strides[0]) = (vx_uint8)roundf(sum[2] / wsum[2]);
                *((vx_uint8 *)dst + y * src_strides[1] + (x + 3) * src_strides[0]) = (vx_uint8)roundf(sum[3] / wsum[3]);
            }
        }
    }
    else
    {
        for (y = low_y ; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x += 4)
            {
                float32x4_t v_sum_b = vdupq_n_f32(0.0f), v_sum_g = vdupq_n_f32(0.0f), v_sum_r = vdupq_n_f32(0.0f), v_wsum = vdupq_n_f32(0.0f);
                float32x4_t v_b0, v_g0, v_r0;
                v_b0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1]), v_b0, 0);
                v_b0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 1) * src_strides[1]), v_b0, 1);
                v_b0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 2) * src_strides[1]), v_b0, 2);
                v_b0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 3) * src_strides[1]), v_b0, 3);

                v_g0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + src_strides[0]), v_g0, 0);
                v_g0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 1) * src_strides[1] + src_strides[0]), v_g0, 1);
                v_g0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 2) * src_strides[1] + src_strides[0]), v_g0, 2);
                v_g0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 3) * src_strides[1] + src_strides[0]), v_g0, 3);

                v_r0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]), v_r0, 0);
                v_r0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 1) * src_strides[1] + 2 * src_strides[0]), v_r0, 1);
                v_r0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 2) * src_strides[1] + 2 * src_strides[0]), v_r0, 2);
                v_r0 = vsetq_lane_f32(*((vx_uint8 *)src + y * src_strides[2] + (x + 3) * src_strides[1] + 2 * src_strides[0]), v_r0, 3);

                //kernel filter
                for (vx_int32 k = 0; k < maxk; k++)
                {
                    vx_int32 neighbor_x = x + Radius_x[k];
                    vx_int32 neighbor_x1 = (x + 1) + Radius_x[k];
                    vx_int32 neighbor_x2 = (x + 2) + Radius_x[k];
                    vx_int32 neighbor_x3 = (x + 3) + Radius_x[k];

                    vx_int32 neighbor_y = y + Radius_y[k];
                    vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[1] - 1) ? (dims[1] - 1) : neighbor_x);
                    vx_int32 tmpx1 = neighbor_x1 < 0 ? 0 : (neighbor_x1 >(dims[1] - 1) ? (dims[1] - 1) : neighbor_x1);
                    vx_int32 tmpx2 = neighbor_x2 < 0 ? 0 : (neighbor_x2 >(dims[1] - 1) ? (dims[1] - 1) : neighbor_x2);
                    vx_int32 tmpx3 = neighbor_x3 < 0 ? 0 : (neighbor_x3 >(dims[1] - 1) ? (dims[1] - 1) : neighbor_x3);

                    vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[2] - 1) ? (dims[2] - 1) : neighbor_y);

                    float32x4_t v_b, v_g, v_r;
                    v_b = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1]), v_b, 0);
                    v_b = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx1 * src_strides[1]), v_b, 1);
                    v_b = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx2 * src_strides[1]), v_b, 2);
                    v_b = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx3 * src_strides[1]), v_b, 3);

                    v_g = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + src_strides[0]), v_g, 0);
                    v_g = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx1 * src_strides[1] + src_strides[0]), v_g, 1);
                    v_g = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx2 * src_strides[1] + src_strides[0]), v_g, 2);
                    v_g = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx3 * src_strides[1] + src_strides[0]), v_g, 3);

                    v_r = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 2 * src_strides[0]), v_r, 0);
                    v_r = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx1 * src_strides[1] + 2 * src_strides[0]), v_r, 1);
                    v_r = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx2 * src_strides[1] + 2 * src_strides[0]), v_r, 2);
                    v_r = vsetq_lane_f32(*((vx_uint8 *)src + tmpy * src_strides[2] + tmpx3 * src_strides[1] + 2 * src_strides[0]), v_r, 3);

                    if (neighbor_x < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.U8, v_b, 0);
                        }
                    }
                            
                    if (neighbor_x1 < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.U8, v_b, 1);
                        }
                    }

                    if (neighbor_x2 < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.U8, v_b, 2);
                        }
                    }

                    if (neighbor_x3 < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.U8, v_b, 3);
                        }
                    }

                    vx_int32 weight_k = (Radius_y[k] + radius) * diameter + (Radius_x[k] + radius);
                    float32x4_t space_weight_val = vdupq_n_f32(space_weight[weight_k]);
                    float32x4_t v_abs_val_b = vabdq_f32(v_b, v_b0);
                    float32x4_t v_abs_val_g = vabdq_f32(v_g, v_g0);
                    float32x4_t v_abs_val_r = vabdq_f32(v_r, v_r0);
                    float32x4_t v_abs_val = vaddq_f32(v_abs_val_r, vaddq_f32(v_abs_val_b, v_abs_val_g));
                    vx_int32 abs_val0 = (vx_int32)vgetq_lane_f32(v_abs_val, 0);
                    vx_int32 abs_val1 = (vx_int32)vgetq_lane_f32(v_abs_val, 1);
                    vx_int32 abs_val2 = (vx_int32)vgetq_lane_f32(v_abs_val, 2);
                    vx_int32 abs_val3 = (vx_int32)vgetq_lane_f32(v_abs_val, 3);
                    float32x4_t color_weight_val;
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val0], color_weight_val, 0);
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val1], color_weight_val, 1);
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val2], color_weight_val, 2);
                    color_weight_val = vsetq_lane_f32(color_weight[abs_val3], color_weight_val, 3);
                    float32x4_t w = vmulq_f32(space_weight_val, color_weight_val);

                    v_sum_b = vaddq_f32(v_sum_b, vmulq_f32(v_b, w));
                    v_sum_g = vaddq_f32(v_sum_g, vmulq_f32(v_g, w));
                    v_sum_r = vaddq_f32(v_sum_r, vmulq_f32(v_r, w));
                    v_wsum = vaddq_f32(v_wsum, w);
                }

                vx_float32 sum_b[4], sum_g[4], sum_r[4], wsum[4];
                sum_b[0] = vgetq_lane_f32(v_sum_b, 0);
                sum_b[1] = vgetq_lane_f32(v_sum_b, 1);
                sum_b[2] = vgetq_lane_f32(v_sum_b, 2);
                sum_b[3] = vgetq_lane_f32(v_sum_b, 3);

                sum_g[0] = vgetq_lane_f32(v_sum_g, 0);
                sum_g[1] = vgetq_lane_f32(v_sum_g, 1);
                sum_g[2] = vgetq_lane_f32(v_sum_g, 2);
                sum_g[3] = vgetq_lane_f32(v_sum_g, 3);

                sum_r[0] = vgetq_lane_f32(v_sum_r, 0);
                sum_r[1] = vgetq_lane_f32(v_sum_r, 1);
                sum_r[2] = vgetq_lane_f32(v_sum_r, 2);
                sum_r[3] = vgetq_lane_f32(v_sum_r, 3);

                wsum[0] = vgetq_lane_f32(v_wsum, 0);
                wsum[1] = vgetq_lane_f32(v_wsum, 1);
                wsum[2] = vgetq_lane_f32(v_wsum, 2);
                wsum[3] = vgetq_lane_f32(v_wsum, 3);

                // b
                *((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1]) = (vx_uint8)roundf(sum_b[0] / wsum[0]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 1) * src_strides[1]) = (vx_uint8)roundf(sum_b[1] / wsum[1]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 2) * src_strides[1]) = (vx_uint8)roundf(sum_b[2] / wsum[2]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 3) * src_strides[1]) = (vx_uint8)roundf(sum_b[3] / wsum[3]);

                // g
                *((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + src_strides[0]) = (vx_uint8)roundf(sum_g[0] / wsum[0]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 1) * src_strides[1] + src_strides[0]) = (vx_uint8)roundf(sum_g[1] / wsum[1]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 2) * src_strides[1] + src_strides[0]) = (vx_uint8)roundf(sum_g[2] / wsum[2]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 3) * src_strides[1] + src_strides[0]) = (vx_uint8)roundf(sum_g[3] / wsum[3]);

                // r
                *((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]) = (vx_uint8)roundf(sum_r[0] / wsum[0]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 1) * src_strides[1] + 2 * src_strides[0]) = (vx_uint8)roundf(sum_r[1] / wsum[1]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 2) * src_strides[1] + 2 * src_strides[0]) = (vx_uint8)roundf(sum_r[2] / wsum[2]);
                *((vx_uint8 *)dst + y * src_strides[2] + (x + 3) * src_strides[1] + 2 * src_strides[0]) = (vx_uint8)roundf(sum_r[3] / wsum[3]);
            }
        }
    }

    releaseRes(color_weight);
    releaseRes(space_weight);

    free(Radius_y);
    free(Radius_x);

    return status;
}

static vx_status bilateralFilter_s16(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                                     vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                                     void* dst, vx_size* dst_strides, vx_border_t *bordermode)
{
    vx_status status = VX_FAILURE;
    vx_int32 y = 0, x = 0;
    vx_int32 low_x, low_y, high_x, high_y;
    vx_int32 radius_y, radius_x;
    vx_int32 radius = diameter/2;
    vx_enum border_mode = bordermode->mode;
    vx_uint8 cn = (num_of_dims == 2) ? 1 : 3;
    vx_int16 minVal = -1;
    vx_int16 maxVal = 1;
    vx_int32 kExpNumBinsPerChannel = 1 << 12;
    vx_float64 gauss_color_coeff = -0.5 / (sigma_color * sigma_color);
    vx_float64 gauss_space_coeff = -0.5 / (sigma_space * sigma_space);
    vx_float32 lastExpVal = 1.f;
    vx_float32 len, scale_index;
    vx_int32 kExpNumBins;
    vx_float32 *color_weight, *space_weight;

    getMinMax(src, src_strides, dims, num_of_dims, &maxVal, &minVal);
    if ((vx_float32)(abs(maxVal - minVal)) < FLT_EPSILON)
    {
        if (num_of_dims == 2)
        {
            for (y = 0; y < dims[1]; y++)
            {
                memcpy((vx_int8 *)dst + dst_strides[1] * y,
                       (vx_int8 *)src + src_strides[1] * y,
                       dims[0] * src_strides[0]);
            }
            status = VX_SUCCESS;
        }
        else if (num_of_dims == 3)
        {
            for (y = 0; y < dims[2]; y++)
            {
                for (x = 0; x < dims[1]; x++)
                {
                    memcpy((vx_int8 *)dst + dst_strides[2] * y + dst_strides[1] * x,
                           (vx_int8 *)src + src_strides[2] * y + src_strides[1] * x,
                           dims[0] * src_strides[0]);
                }
            }
            status = VX_SUCCESS;
        }

        return status;
    }

    //calculation color weight
    len = (vx_float32)(maxVal - minVal) * cn;
    kExpNumBins = kExpNumBinsPerChannel * cn;
    color_weight = (vx_float32 *)malloc((kExpNumBins + 2) * sizeof(vx_float32));
    if (NULL == color_weight)
    {
        return VX_ERROR_NO_MEMORY;
    }
    scale_index = kExpNumBins / len;
    for (vx_uint32 i = 0; i < (kExpNumBins + 2); i++)
    {
        if (lastExpVal > 0.f)
        {
            vx_float64 val = i / scale_index;
            color_weight[i] = (vx_float32)exp(val * val * gauss_color_coeff);
            lastExpVal = color_weight[i];
        }
        else
        {
            color_weight[i] = 0.f;
        }
    }
    //calculation space weight
    space_weight = NULL;
    status = calcSpaceWeight(diameter, gauss_space_coeff, &space_weight);
    if (status != VX_SUCCESS)
    {
        free(color_weight);
        return status;
    }

    // Calculate radius
    vx_int32 rx = 0, ry = 0;
    vx_int32 *Radius_y = (vx_int32 *)calloc(diameter * diameter, sizeof(vx_int32));
    vx_int32 *Radius_x = (vx_int32 *)calloc(diameter * diameter, sizeof(vx_int32));
    for (radius_y = -radius; radius_y <= radius; radius_y++)
    {
        for (radius_x = -radius; radius_x <= radius; radius_x++)
        {
            vx_float64 r = sqrt((vx_float64)radius_y * radius_y + (vx_float64)radius_x * radius_x);
            if (r > radius)
            {
                continue;
            }
            Radius_y[ry++] = radius_y;
            Radius_x[rx++] = radius_x;
        }
    }
    vx_int32 maxk = rx;

    if (bordermode->mode == VX_BORDER_UNDEFINED)
    {
        low_x = radius;
        high_x = (dims[num_of_dims - 2] >= radius) ? dims[num_of_dims - 2] - radius : 0;
        low_y = radius;
        high_y = (dims[num_of_dims - 1] >= radius) ? dims[num_of_dims - 1] - radius : 0;
    }
    else
    {
        low_x = 0;
        high_x = dims[num_of_dims - 2];
        low_y = 0;
        high_y = dims[num_of_dims - 1];
    }

    if (num_of_dims == 2)
    {
        for (y = low_y; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x += 4)
            {
                float32x4_t v_sum = vdupq_n_f32(0.0f), v_wsum = vdupq_n_f32(0.0f);
                float32x4_t value;
                value = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + y * src_strides[1] + x * src_strides[0]), value, 0);
                value = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + y * src_strides[1] + (x + 1) * src_strides[0]), value, 1);
                value = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + y * src_strides[1] + (x + 2) * src_strides[0]), value, 2);
                value = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + y * src_strides[1] + (x + 3) * src_strides[0]), value, 3);
                //kernel filter
                for (vx_int32 k = 0; k < maxk; k++)
                {
                    vx_int32 neighbor_x = x + Radius_x[k];
                    vx_int32 neighbor_x1 = (x + 1) + Radius_x[k];
                    vx_int32 neighbor_x2 = (x + 2) + Radius_x[k];
                    vx_int32 neighbor_x3 = (x + 3) + Radius_x[k];

                    vx_int32 neighbor_y = y + Radius_y[k];
                    vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[0] - 1) ? (dims[0] - 1) : neighbor_x);
                    vx_int32 tmpx1 = neighbor_x1 < 0 ? 0 : (neighbor_x1 >(dims[0] - 1) ? (dims[0] - 1) : neighbor_x1);
                    vx_int32 tmpx2 = neighbor_x2 < 0 ? 0 : (neighbor_x2 >(dims[0] - 1) ? (dims[0] - 1) : neighbor_x2);
                    vx_int32 tmpx3 = neighbor_x3 < 0 ? 0 : (neighbor_x3 >(dims[0] - 1) ? (dims[0] - 1) : neighbor_x3);

                    vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[1] - 1) ? (dims[1] - 1) : neighbor_y);

                    float32x4_t neighborVal;
                    neighborVal = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + tmpy * src_strides[1] + tmpx * src_strides[0]), neighborVal, 0);
                    neighborVal = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + tmpy * src_strides[1] + tmpx1 * src_strides[0]), neighborVal, 1);
                    neighborVal = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + tmpy * src_strides[1] + tmpx2 * src_strides[0]), neighborVal, 2);
                    neighborVal = vsetq_lane_f32(*(vx_int16 *)((vx_int8 *)src + tmpy * src_strides[1] + tmpx3 * src_strides[0]), neighborVal, 3);

                    if (neighbor_x < 0 || neighbor_y < 0)
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.S16, neighborVal, 0);

                    if (neighbor_x1 < 0 || neighbor_y < 0)
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.S16, neighborVal, 1);

                    if (neighbor_x2 < 0 || neighbor_y < 0)
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.S16, neighborVal, 2);

                    if (neighbor_x3 < 0 || neighbor_y < 0)
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                            neighborVal = vsetq_lane_f32(bordermode->constant_value.S16, neighborVal, 3);

                    vx_int32 weight_k = (Radius_y[k] + radius) * diameter + (Radius_x[k] + radius);
                    float32x4_t space_weight_val = vdupq_n_f32(space_weight[weight_k]);

                    float32x4_t v_abs_val = vabdq_f32(neighborVal, value);
                    float32x4_t v_scale_index = vdupq_n_f32(scale_index);
                    float32x4_t v_alpha = vmulq_f32(v_abs_val, v_scale_index);
                    vx_float32 alpha0 = vgetq_lane_f32(v_alpha, 0);
                    vx_float32 alpha1 = vgetq_lane_f32(v_alpha, 1);
                    vx_float32 alpha2 = vgetq_lane_f32(v_alpha, 2);
                    vx_float32 alpha3 = vgetq_lane_f32(v_alpha, 3);
                    vx_int32 idx0 = (vx_int32)floorf(alpha0);
                    vx_int32 idx1 = (vx_int32)floorf(alpha1);
                    vx_int32 idx2 = (vx_int32)floorf(alpha2);
                    vx_int32 idx3 = (vx_int32)floorf(alpha3);
                    alpha0 -= idx0;
                    alpha1 -= idx1;
                    alpha2 -= idx2;
                    alpha3 -= idx3;

                    float32x4_t color_weight_val;
                    color_weight_val = vsetq_lane_f32(color_weight[idx0] + alpha0 * (color_weight[idx0 + 1] - color_weight[idx0]), color_weight_val, 0);
                    color_weight_val = vsetq_lane_f32(color_weight[idx1] + alpha1 * (color_weight[idx1 + 1] - color_weight[idx1]), color_weight_val, 1);
                    color_weight_val = vsetq_lane_f32(color_weight[idx2] + alpha2 * (color_weight[idx2 + 1] - color_weight[idx2]), color_weight_val, 2);
                    color_weight_val = vsetq_lane_f32(color_weight[idx3] + alpha3 * (color_weight[idx3 + 1] - color_weight[idx3]), color_weight_val, 3);
                    float32x4_t w = vmulq_f32(space_weight_val, color_weight_val);

                    v_sum = vaddq_f32(v_sum, vmulq_f32(neighborVal, w));
                    v_wsum = vaddq_f32(v_wsum, w);
                }

                vx_float32 sum[4], wsum[4];
                sum[0] = vgetq_lane_f32(v_sum, 0);
                sum[1] = vgetq_lane_f32(v_sum, 1);
                sum[2] = vgetq_lane_f32(v_sum, 2);
                sum[3] = vgetq_lane_f32(v_sum, 3);
                wsum[0] = vgetq_lane_f32(v_wsum, 0);
                wsum[1] = vgetq_lane_f32(v_wsum, 1);
                wsum[2] = vgetq_lane_f32(v_wsum, 2);
                wsum[3] = vgetq_lane_f32(v_wsum, 3);

                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[1] + x * src_strides[0]) = (vx_int16)roundf(sum[0] / wsum[0]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[1] + (x + 1) * src_strides[0]) = (vx_int16)roundf(sum[1] / wsum[1]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[1] + (x + 2) * src_strides[0]) = (vx_int16)roundf(sum[2] / wsum[2]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[1] + (x + 3) * src_strides[0]) = (vx_int16)roundf(sum[3] / wsum[3]);
            }
        }
    }
    else if (num_of_dims == 3)
    {
        for (y = low_y; y < high_y; y++)
        {
            for (x = low_x; x < high_x; x += 4)
            {
                float32x4_t v_sum_b = vdupq_n_f32(0.0f), v_sum_g = vdupq_n_f32(0.0f), v_sum_r = vdupq_n_f32(0.0f), v_wsum = vdupq_n_f32(0.0f);
                float32x4_t v_b0, v_g0, v_r0;
                v_b0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1]), v_b0, 0);
                v_b0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 1) * src_strides[1]), v_b0, 1);
                v_b0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 2) * src_strides[1]), v_b0, 2);
                v_b0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 3) * src_strides[1]), v_b0, 3);

                v_g0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + src_strides[0]), v_g0, 0);
                v_g0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 1) * src_strides[1] + src_strides[0]), v_g0, 1);
                v_g0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 2) * src_strides[1] + src_strides[0]), v_g0, 2);
                v_g0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 3) * src_strides[1] + src_strides[0]), v_g0, 3);

                v_r0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]), v_r0, 0);
                v_r0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 1) * src_strides[1] + 2 * src_strides[0]), v_r0, 1);
                v_r0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 2) * src_strides[1] + 2 * src_strides[0]), v_r0, 2);
                v_r0 = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + (x + 3) * src_strides[1] + 2 * src_strides[0]), v_r0, 3);

                //kernel filter
                for (vx_int32 k = 0; k < maxk; k++)
                {
                    vx_int32 neighbor_x = x + Radius_x[k];
                    vx_int32 neighbor_x1 = (x + 1) + Radius_x[k];
                    vx_int32 neighbor_x2 = (x + 2) + Radius_x[k];
                    vx_int32 neighbor_x3 = (x + 3) + Radius_x[k];

                    vx_int32 neighbor_y = y + Radius_y[k];
                    vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[1] - 1) ? (dims[1] - 1) : neighbor_x);
                    vx_int32 tmpx1 = neighbor_x1 < 0 ? 0 : (neighbor_x1 >(dims[1] - 1) ? (dims[1] - 1) : neighbor_x1);
                    vx_int32 tmpx2 = neighbor_x2 < 0 ? 0 : (neighbor_x2 >(dims[1] - 1) ? (dims[1] - 1) : neighbor_x2);
                    vx_int32 tmpx3 = neighbor_x3 < 0 ? 0 : (neighbor_x3 >(dims[1] - 1) ? (dims[1] - 1) : neighbor_x3);

                    vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[2] - 1) ? (dims[2] - 1) : neighbor_y);

                    float32x4_t v_b, v_g, v_r;
                    v_b = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1]), v_b, 0);
                    v_b = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx1 * src_strides[1]), v_b, 1);
                    v_b = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx2 * src_strides[1]), v_b, 2);
                    v_b = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx3 * src_strides[1]), v_b, 3);

                    v_g = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + src_strides[0]), v_g, 0);
                    v_g = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx1 * src_strides[1] + src_strides[0]), v_g, 1);
                    v_g = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx2 * src_strides[1] + src_strides[0]), v_g, 2);
                    v_g = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx3 * src_strides[1] + src_strides[0]), v_g, 3);

                    v_r = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 2 * src_strides[0]), v_r, 0);
                    v_r = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx1 * src_strides[1] + 2 * src_strides[0]), v_r, 1);
                    v_r = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx2 * src_strides[1] + 2 * src_strides[0]), v_r, 2);
                    v_r = vsetq_lane_f32(*(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx3 * src_strides[1] + 2 * src_strides[0]), v_r, 3);

                    if (neighbor_x < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.S16, v_b, 0);
                        }
                    }
                            
                    if (neighbor_x1 < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.S16, v_b, 1);
                        }
                    }

                    if (neighbor_x2 < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.S16, v_b, 2);
                        }
                    }

                    if (neighbor_x3 < 0 || neighbor_y < 0)
                    {
                        if (border_mode == VX_BORDER_MODE_CONSTANT)
                        {
                            v_g = v_r = v_b = vsetq_lane_f32(bordermode->constant_value.S16, v_b, 3);
                        }
                    }

                    vx_int32 weight_k = (Radius_y[k] + radius) * diameter + (Radius_x[k] + radius);
                    float32x4_t space_weight_val = vdupq_n_f32(space_weight[weight_k]);

                    float32x4_t v_abs_val_b = vabdq_f32(v_b, v_b0);
                    float32x4_t v_abs_val_g = vabdq_f32(v_g, v_g0);
                    float32x4_t v_abs_val_r = vabdq_f32(v_r, v_r0);
                    float32x4_t v_abs_val = vaddq_f32(v_abs_val_r, vaddq_f32(v_abs_val_b, v_abs_val_g));
                    float32x4_t v_scale_index = vdupq_n_f32(scale_index);
                    float32x4_t v_alpha = vmulq_f32(v_abs_val, v_scale_index);
                    vx_float32 alpha0 = vgetq_lane_f32(v_alpha, 0);
                    vx_float32 alpha1 = vgetq_lane_f32(v_alpha, 1);
                    vx_float32 alpha2 = vgetq_lane_f32(v_alpha, 2);
                    vx_float32 alpha3 = vgetq_lane_f32(v_alpha, 3);
                    vx_int32 idx0 = (vx_int32)floorf(alpha0);
                    vx_int32 idx1 = (vx_int32)floorf(alpha1);
                    vx_int32 idx2 = (vx_int32)floorf(alpha2);
                    vx_int32 idx3 = (vx_int32)floorf(alpha3);
                    alpha0 -= idx0;
                    alpha1 -= idx1;
                    alpha2 -= idx2;
                    alpha3 -= idx3;

                    float32x4_t color_weight_val;
                    color_weight_val = vsetq_lane_f32(color_weight[idx0] + alpha0 * (color_weight[idx0 + 1] - color_weight[idx0]), color_weight_val, 0);
                    color_weight_val = vsetq_lane_f32(color_weight[idx1] + alpha1 * (color_weight[idx1 + 1] - color_weight[idx1]), color_weight_val, 1);
                    color_weight_val = vsetq_lane_f32(color_weight[idx2] + alpha2 * (color_weight[idx2 + 1] - color_weight[idx2]), color_weight_val, 2);
                    color_weight_val = vsetq_lane_f32(color_weight[idx3] + alpha3 * (color_weight[idx3 + 1] - color_weight[idx3]), color_weight_val, 3);
                    float32x4_t w = vmulq_f32(space_weight_val, color_weight_val);

                    v_sum_b = vaddq_f32(v_sum_b, vmulq_f32(v_b, w));
                    v_sum_g = vaddq_f32(v_sum_g, vmulq_f32(v_g, w));
                    v_sum_r = vaddq_f32(v_sum_r, vmulq_f32(v_r, w));
                    v_wsum = vaddq_f32(v_wsum, w);
                }

                vx_float32 sum_b[4], sum_g[4], sum_r[4], wsum[4];
                sum_b[0] = vgetq_lane_f32(v_sum_b, 0);
                sum_b[1] = vgetq_lane_f32(v_sum_b, 1);
                sum_b[2] = vgetq_lane_f32(v_sum_b, 2);
                sum_b[3] = vgetq_lane_f32(v_sum_b, 3);

                sum_g[0] = vgetq_lane_f32(v_sum_g, 0);
                sum_g[1] = vgetq_lane_f32(v_sum_g, 1);
                sum_g[2] = vgetq_lane_f32(v_sum_g, 2);
                sum_g[3] = vgetq_lane_f32(v_sum_g, 3);

                sum_r[0] = vgetq_lane_f32(v_sum_r, 0);
                sum_r[1] = vgetq_lane_f32(v_sum_r, 1);
                sum_r[2] = vgetq_lane_f32(v_sum_r, 2);
                sum_r[3] = vgetq_lane_f32(v_sum_r, 3);

                wsum[0] = vgetq_lane_f32(v_wsum, 0);
                wsum[1] = vgetq_lane_f32(v_wsum, 1);
                wsum[2] = vgetq_lane_f32(v_wsum, 2);
                wsum[3] = vgetq_lane_f32(v_wsum, 3);

                // b
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1]) = (vx_int16)roundf(sum_b[0] / wsum[0]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 1) * src_strides[1]) = (vx_int16)roundf(sum_b[1] / wsum[1]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 2) * src_strides[1]) = (vx_int16)roundf(sum_b[2] / wsum[2]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 3) * src_strides[1]) = (vx_int16)roundf(sum_b[3] / wsum[3]);

                // g
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + src_strides[0]) = (vx_int16)roundf(sum_g[0] / wsum[0]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 1) * src_strides[1] + src_strides[0]) = (vx_int16)roundf(sum_g[1] / wsum[1]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 2) * src_strides[1] + src_strides[0]) = (vx_int16)roundf(sum_g[2] / wsum[2]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 3) * src_strides[1] + src_strides[0]) = (vx_int16)roundf(sum_g[3] / wsum[3]);

                // r
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]) = (vx_int16)roundf(sum_r[0] / wsum[0]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 1) * src_strides[1] + 2 * src_strides[0]) = (vx_int16)roundf(sum_r[1] / wsum[1]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 2) * src_strides[1] + 2 * src_strides[0]) = (vx_int16)roundf(sum_r[2] / wsum[2]);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + (x + 3) * src_strides[1] + 2 * src_strides[0]) = (vx_int16)roundf(sum_r[3] / wsum[3]);
            }
        }
    }

    free(color_weight);
    releaseRes(space_weight);

    free(Radius_y);
    free(Radius_x);

    return status;
}

vx_status vxBilateralFilter(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                            vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                            void* dst, vx_size* dst_strides, vx_enum type, vx_border_t *bordermode)
{
    vx_status status = VX_SUCCESS;
    // In case of 3 dimensions the 1st dimension of the vx_tensor. Which can be of size 1 or 2.
    if ((num_of_dims != 3 && num_of_dims != 2) || (num_of_dims == 3 && (dims[0] != 1 && dims[0] != 2)))
    {
        return VX_ERROR_INVALID_PARAMETERS;
    }

    if(type == VX_TYPE_UINT8)
    {
        status = bilateralFilter_8u(src, src_strides, dims, num_of_dims, diameter, sigma_space, sigma_color, dst, dst_strides, bordermode);
    }
    else if(type == VX_TYPE_INT16)
    {
        status = bilateralFilter_s16(src, src_strides, dims, num_of_dims, diameter, sigma_space, sigma_color, dst, dst_strides, bordermode);
    }

    return status;
}
