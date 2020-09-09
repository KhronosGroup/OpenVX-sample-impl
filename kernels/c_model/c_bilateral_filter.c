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
#include <c_model.h>

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

    for (y = low_y ; y < high_y; y++)
    {
        if (num_of_dims == 2)
        {
            for (x = low_x; x < high_x; x++)
            {
                vx_float32 sum = 0, wsum = 0;
                vx_uint8 val0 = *((vx_uint8 *)src + y * src_strides[1] + x * src_strides[0]);
                //kernel filter
                for (radius_y = -radius; radius_y <= radius; radius_y++)
                {
                    for (radius_x = -radius; radius_x <= radius; radius_x++)
                    {
                        vx_float64 r = sqrt((vx_float64)radius_y * radius_y + (vx_float64)radius_x * radius_x);
                        if (r > radius)
                        {
                            continue;
                        }
                        vx_int32 neighbor_x = x + radius_x;
                        vx_int32 neighbor_y = y + radius_y;
                        vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[0] - 1) ? (dims[0] - 1) : neighbor_x);
                        vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[1] - 1) ? (dims[1] - 1) : neighbor_y);
                        vx_uint8 neighborVal = 0;
                        neighborVal = *((vx_uint8 *)src + tmpy * src_strides[1] + tmpx * src_strides[0]);
                        if (neighbor_x < 0 || neighbor_y < 0 || neighbor_x >= dims[0] || neighbor_y >= dims[1])
                        {
                            if (border_mode == VX_BORDER_MODE_CONSTANT)
                            {
                                neighborVal = bordermode->constant_value.U8;
                            }
                        }
                        vx_float32 w = space_weight[(radius_y + radius) * diameter + (radius_x + radius)] *
                                       color_weight[abs(neighborVal - val0)];
                        sum += neighborVal * w;
                        wsum += w;
                    }
                }

                *((vx_uint8 *)dst + y * src_strides[1] + x * src_strides[0]) = (vx_uint8)roundf(sum / wsum);
            }
        }
        else
        {
            for (x = low_x; x < high_x; x++)
            {
                vx_float32 sum_b = 0, sum_g = 0, sum_r = 0, wsum = 0;
                vx_uint8 b0 = *((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 0 * src_strides[0]);
                vx_uint8 g0 = *((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 1 * src_strides[0]);
                vx_uint8 r0 = *((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]);
                //kernel filter
                for (radius_y = -radius; radius_y <= radius; radius_y++)
                {
                    for (radius_x = -radius; radius_x <= radius; radius_x++)
                    {
                        vx_float64 dist = sqrt((vx_float64)radius_y * radius_y + (vx_float64)radius_x * radius_x);
                        if (dist > radius)
                        {
                            continue;
                        }
                        vx_int32 neighbor_x = x + radius_x;
                        vx_int32 neighbor_y = y + radius_y;
                        vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[1] - 1) ? (dims[1] - 1) : neighbor_x);
                        vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[2] - 1) ? (dims[2] - 1) : neighbor_y);
                        vx_uint8 b = 0, g = 0, r = 0;
                        b = *((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 0 * src_strides[0]);
                        g = *((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 1 * src_strides[0]);
                        r = *((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 2 * src_strides[0]);
                        if (neighbor_x < 0 || neighbor_y < 0)
                        {
                            if (border_mode == VX_BORDER_MODE_CONSTANT)
                            {
                                b = g = r = bordermode->constant_value.U8;
                            }
                        }
                        vx_float32 w = space_weight[(radius_y + radius) * diameter + (radius_x + radius)] *
                                       color_weight[abs(b - b0) + abs(g - g0) + abs(r - r0)];
                        sum_b += b * w;
                        sum_g += g * w;
                        sum_r += r * w;
                        wsum += w;
                    }
                }

                *((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 0 * src_strides[0]) = (vx_uint8)roundf(sum_b / wsum);
                *((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 1 * src_strides[0]) = (vx_uint8)roundf(sum_g / wsum);
                *((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]) = (vx_uint8)roundf(sum_r / wsum);
            }
        }
    }

    releaseRes(color_weight);
    releaseRes(space_weight);

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

    for (y = low_y; y < high_y; y++)
    {
        if (num_of_dims == 2)
        {
            for (x = low_x; x < high_x; x++)
            {
                vx_int16 val0 = *(vx_int16 *)((vx_int8 *)src + y * src_strides[1] + x * src_strides[0]);
                vx_float32 sum = 0, wsum = 0;
                //kernel filter
                for (radius_y = -radius; radius_y <= radius; radius_y++)
                {
                    for (radius_x = -radius; radius_x <= radius; radius_x++)
                    {
                        vx_float64 r = sqrt((vx_float64)radius_y * radius_y + (vx_float64)radius_x * radius_x);
                        if (r > radius)
                        {
                            continue;
                        }
                        vx_int32 neighbor_x = x + radius_x;
                        vx_int32 neighbor_y = y + radius_y;
                        vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[0] - 1) ? (dims[0] - 1) : neighbor_x);
                        vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[1] - 1) ? (dims[1] - 1) : neighbor_y);
                        vx_int16 val = *(vx_int16 *)((vx_int8 *)src + tmpy * src_strides[1] + tmpx * src_strides[0]);
                        if (neighbor_x < 0 || neighbor_y < 0)
                        {
                            if (border_mode == VX_BORDER_MODE_CONSTANT)
                            {
                                val = bordermode->constant_value.S16;
                            }
                        }

                        vx_float32 alpha = abs(val - val0) * scale_index;
                        vx_int32 idx = (vx_int32)floorf(alpha);
                        alpha -= idx;
                        vx_float32 w = space_weight[(radius_y + radius) * diameter + (radius_x + radius)] *
                            (color_weight[idx] + alpha * (color_weight[idx + 1] - color_weight[idx]));
                        sum += val * w;
                        wsum += w;
                    }
                }

                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[1] + x * src_strides[0]) = (vx_int16)roundf(sum / wsum);
            }
        }
        else if (num_of_dims == 3)
        {
            for (x = low_x; x < high_x; x++)
            {
                vx_float32 sum_b = 0, sum_g = 0, sum_r = 0, wsum = 0;
                vx_int16 b0 = *(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 0 * src_strides[0]);
                vx_int16 g0 = *(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 1 * src_strides[0]);
                vx_int16 r0 = *(vx_int16 *)((vx_uint8 *)src + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]);
                //kernel filter
                for (radius_y = -radius; radius_y <= radius; radius_y++)
                {
                    for (radius_x = -radius; radius_x <= radius; radius_x++)
                    {
                        vx_float64 dist = sqrt((vx_float64)radius_y * radius_y + (vx_float64)radius_x * radius_x);
                        if (dist > radius)
                        {
                            continue;
                        }
                        vx_int32 neighbor_x = x + radius_x;
                        vx_int32 neighbor_y = y + radius_y;
                        vx_int32 tmpx = neighbor_x < 0 ? 0 : (neighbor_x > (dims[1] - 1) ? (dims[1] - 1) : neighbor_x);
                        vx_int32 tmpy = neighbor_y < 0 ? 0 : (neighbor_y > (dims[2] - 1) ? (dims[2] - 1) : neighbor_y);
                        vx_int16 b = *(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 0 * src_strides[0]);
                        vx_int16 g = *(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 1 * src_strides[0]);
                        vx_int16 r = *(vx_int16 *)((vx_uint8 *)src + tmpy * src_strides[2] + tmpx * src_strides[1] + 2 * src_strides[0]);
                        if (neighbor_x < 0 || neighbor_y < 0)
                        {
                            if (border_mode == VX_BORDER_MODE_CONSTANT)
                            {
                                b = g = r = bordermode->constant_value.S16;
                            }
                        }

                        vx_float32 alpha = (abs(b- b0) + abs(g - g0) + abs(r - r0)) * scale_index;
                        vx_int32 idx = (vx_int32)floorf(alpha);
                        alpha -= idx;
                        vx_float32 w = space_weight[(radius_y + radius) * diameter + (radius_x + radius)] *
                            (color_weight[idx] + alpha * (color_weight[idx + 1] - color_weight[idx]));
                        sum_b += b * w;
                        sum_g += g * w;
                        sum_r += r * w;
                        wsum += w;
                    }
                }

                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 0 * src_strides[0]) = (vx_int16)roundf(sum_b / wsum);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 1 * src_strides[0]) = (vx_int16)roundf(sum_g / wsum);
                *(vx_int16 *)((vx_uint8 *)dst + y * src_strides[2] + x * src_strides[1] + 2 * src_strides[0]) = (vx_int16)roundf(sum_r / wsum);
            }
        }
    }

    free(color_weight);
    releaseRes(space_weight);

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
