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
#include <c_model.h>
#include "tensor_utils.h"


static vx_status bilateralFilter_8u(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                                    vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                                    void* dst, vx_size* dst_strides)
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0, j = 0;
    vx_int32 radius = diameter/2;
    vx_size out_size = ComputeNumberOfElements(dims, num_of_dims);
    vx_float32 color_weight[256];
    vx_float32 gauss_color_coeff = -0.5f/(sigma_color*sigma_color);
    vx_float32 gauss_space_coeff = -0.5f/(sigma_space*sigma_space);

    vx_float32 *space_weight = (vx_float32*)malloc((radius * 2 + 1) * sizeof(vx_float32));
    if(!space_weight)
    {
      status = VX_ERROR_NO_MEMORY;
      return status;
    }

    if(radius < 1)
    {
        radius = 1;
    }

    if(radius > (vx_int32)out_size)
    {
        radius = (vx_uint32)out_size;
    }

    for(i = 0; i < 256; i++)
    {
        color_weight[i] = (vx_float32)exp(i*i*gauss_color_coeff);
    }

    for(i = -radius; i <= radius; i++ )
    {
         space_weight[i+radius] = (vx_float32)exp(i*i*gauss_space_coeff);
    }

    vx_size src_pos = 0, dst_pos = 0, nei_pos = 0;
    vx_int8 *src_ptr = NULL, *dst_ptr = NULL, *nei_ptr = NULL;
    vx_float32 sum = 0, wsum = 0, w = 0;

    for (i = radius; i < (vx_int32)(out_size-radius); i++)
    {
        vx_size src_pos0 = 0, dst_pos0 = 0;
        src_pos = ComputeGlobalPositionsFromIndex(i, dims, src_strides, num_of_dims, &src_pos0);
        dst_pos = ComputeGlobalPositionsFromIndex(i, dims, dst_strides, num_of_dims, &dst_pos0);

        src_ptr = (vx_int8*)src+src_pos;
        dst_ptr = (vx_int8*)dst+dst_pos;

        sum = 0, wsum = 0;

        for(j = -radius; j <= radius; j++)
        {
            vx_size nei_pos0 = 0;
            nei_pos = ComputeGlobalPositionsFromIndex(i+j, dims, src_strides, num_of_dims, &nei_pos0);
            nei_ptr = (vx_int8*)src + nei_pos;
            w = space_weight[j+radius]*color_weight[abs(*nei_ptr - *src_ptr)];
            sum += (*nei_ptr)*w;
            wsum += w;
        }
        *dst_ptr = (vx_int8)round(sum/wsum);
    }

    free(space_weight);

    return status;
}

static vx_status bilateralFilter_16(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                                    vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                                    void* dst, vx_size* dst_strides)
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0, j = 0;
    vx_int32 radius = diameter/2;
    vx_size out_size = ComputeNumberOfElements(dims, num_of_dims);
    vx_float32 *color_weight = (vx_float32 *)calloc(256*256, sizeof(vx_float32));
    if(!color_weight) {
        return VX_ERROR_NO_MEMORY;
    }
    vx_float32 gauss_color_coeff = -0.5f/(sigma_color*sigma_color);
    vx_float32 gauss_space_coeff = -0.5f/(sigma_space*sigma_space);

    vx_float32 *space_weight = (vx_float32*)malloc((radius * 2 + 1) * sizeof(vx_float32));
    if(!space_weight)
    {
        free(color_weight);
        status = VX_ERROR_NO_MEMORY;
        return status;
    }

    if(radius < 1)
    {
        radius = 1;
    }

    if(radius > (vx_int32)out_size)
    {
        radius = (vx_int32)out_size;
    }

    for(i = 0; i < 256*256; i++)
    {
        color_weight[i] = (vx_float32)exp(i*i*gauss_color_coeff);
    }

    for(i = -radius; i <= radius; i++ )
    {
         space_weight[i+radius] = (vx_float32)exp(i*i*gauss_space_coeff);
    }

    vx_size src_pos = 0, dst_pos = 0, nei_pos = 0;
    vx_int16 *src_ptr = NULL, *dst_ptr = NULL, *nei_ptr = NULL;
    vx_float32 sum = 0, wsum = 0, w = 0;

    for (i = radius; i < (vx_int32)(out_size - radius); i++)
    {
        src_pos = ComputeGlobalPositionsFromIndex(i, dims, src_strides, num_of_dims, &src_pos);
        dst_pos = ComputeGlobalPositionsFromIndex(i, dims, dst_strides, num_of_dims, &dst_pos);

        src_ptr = (vx_int16*)((vx_uint8*)src+src_pos);
        dst_ptr = (vx_int16*)((vx_uint8*)dst+dst_pos);

        sum = 0, wsum = 0;

        for(j = -radius; j <= radius; j++)
        {
            nei_pos = ComputeGlobalPositionsFromIndex(i+j, dims, src_strides, num_of_dims, &nei_pos);
            nei_ptr = (vx_int16*)((vx_uint8*)src + nei_pos);
            w = space_weight[j+radius]*color_weight[abs(*nei_ptr - *src_ptr)];
            sum += (*nei_ptr)*w;
            wsum += w;
        }
        *dst_ptr = (vx_int16) round(sum/wsum);
    }
    
    free(space_weight);
    free(color_weight);
    return status;
}

vx_status vxBilateralFilter(void* src, vx_size* src_strides, vx_size* dims, vx_size num_of_dims,
                            vx_int32 diameter, vx_float32 sigma_space, vx_float32 sigma_color,
                            void* dst, vx_size* dst_strides, vx_enum type)
{
    vx_status status = VX_SUCCESS;
    if(type == VX_TYPE_UINT8)
    {
        status = bilateralFilter_8u(src, src_strides, dims, num_of_dims, diameter, sigma_space, sigma_color, dst, dst_strides);
    }
    else if(type == VX_TYPE_INT16)
    {
        status = bilateralFilter_16(src, src_strides, dims, num_of_dims, diameter, sigma_space, sigma_color, dst, dst_strides);
    }

    return status;
}
