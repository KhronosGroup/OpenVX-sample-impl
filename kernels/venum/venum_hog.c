/*
* Copyright (c) 2012-2017 The Khronos Group Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and/or associated documentation files (the
* "Materials"), to deal in the Materials without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Materials, and to
* permit persons to whom the Materials are furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Materials.
*
* MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
* KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
* SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
*    https://www.khronos.org/registry/
*
* THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include <venum.h>
#include <arm_neon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tensor_utils.h"

#define min(a,b) (a<b?a:b)
// nodeless version of the Hog Cells kernel
vx_status vxHogCells(vx_image img, vx_scalar cell_width, vx_scalar cell_height, vx_scalar num_bins, vx_tensor magnitudes, vx_tensor bins)
{
    vx_status status = VX_SUCCESS;
    vx_float32 gx;
    vx_float32 gy;
    vx_float32 orientation;
    vx_float32 magnitude;
    vx_int8 bin;
    vx_int32 width, height;

    vx_int32 cell_w, cell_h, num_orientations;
    status |= vxCopyScalar(cell_width, &cell_w, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(cell_height, &cell_h, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(num_bins, &num_orientations, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    void *src_base = NULL;
    vx_imagepatch_addressing_t src_addr;
    vx_rectangle_t rect;
    status |= vxGetValidRegionImage(img, &rect);
    vx_map_id map_id_src = 0;
    status |= vxMapImagePatch(img, &rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    width = src_addr.dim_x;
    height = src_addr.dim_y;

    void* magnitudes_data = NULL;
    void* bins_data = NULL;
    vx_size magnitudes_dim_num = 0, magnitudes_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, magnitudes_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };
    vx_size bins_dim_num = 0, bins_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, bins_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };

    status |= AllocatePatch(magnitudes, &magnitudes_dim_num, magnitudes_dims, magnitudes_strides, &magnitudes_data, VX_WRITE_ONLY);
    status |= AllocatePatch(bins, &bins_dim_num, bins_dims, bins_strides, &bins_data, VX_WRITE_ONLY);

    vx_float32 num_div_360 = (float)num_orientations / 360.0f;
    vx_int32 num_cellw = (vx_int32)floor(((vx_float64)width) / ((vx_float64)cell_w));// can vcvts_s32_f32()
    
    vx_float32 gx_0, gx_1, gx_2, gx_3;
    vx_float32 gy_0, gy_1, gy_2, gy_3;
    float32x4_t magnitude_f32x4;
    float32x4_t orientation_f32x4;
    float32x4_t fv_0_5_32x4 = vdupq_n_f32(0.5f);
    float32x4_t num_div_360_f32x4 = vdupq_n_f32(num_div_360);
    int32x4_t bin_s32x4;
    vx_int32 cell_wxh = cell_w*cell_h;
    int32x4_t num_orientations_s32x4 = vdupq_n_s32(num_orientations);
    int32x4_t num_cellw_s32x4 = vdupq_n_s32(num_cellw);
    float32_t pi_3_14 = 180 / 3.14159265;
    vx_int32 roiw4 = width >= 3 ? width - 3 : 0;
    for (vx_int32 j = 0; j < height; j++) {
        
        int32x4_t celly_s32x4 = vdupq_n_s32(j/cell_h);
        vx_int32 y1 = j - 1 < 0 ? 0 : j - 1;
        vx_int32 y2 = j + 1 >= height ? height - 1 : j + 1;
        vx_uint8 *src_base_y = (vx_uint8 *)src_base + j*src_addr.stride_y;
        vx_uint8 *src_base_y_y1 = (vx_uint8 *)src_base + y1*src_addr.stride_y;
        vx_uint8 *src_base_y_y2 = (vx_uint8 *)src_base + y2*src_addr.stride_y;
        int i = 0;
        for (; i < roiw4; i+=4) 
        {            
            vx_int32 x1 = i - 1 < 0 ? 0 : i - 1;
            vx_int32 x2 = i + 1 >= width ? width - 1 : i + 1;
            gx_0 = *(src_base_y + x2) - *(src_base_y + x1); 
                    
            x1 = i < 0 ? 0 : i;
            x2 = i + 2 >= width ? width - 1 : i+2;
            gx_1 = *(src_base_y + x2) - *(src_base_y + x1);
            
            x1 = i + 1 < 0 ? 0 : i + 1;
            x2 = i+3 >= width ? width - 1 : i+3;
            gx_2 = *(src_base_y + x2) - *(src_base_y + x1); 
            
            x1 = i+2 < 0 ? 0 : i+2;
            x2 = i+4 >= width ? width - 1 : i+4;
            gx_3 = *(src_base_y + x2) - *(src_base_y + x1); 

            gy_0 = *(src_base_y_y2 + i) - *(src_base_y_y1 + i);
            gy_1 = *(src_base_y_y2 + i + 1) - *(src_base_y_y1 + i + 1);
            gy_2 = *(src_base_y_y2 + i + 2) - *(src_base_y_y1 + i + 2);
            gy_3 = *(src_base_y_y2 + i + 3) - *(src_base_y_y1 + i + 3);
                        
            //calculating mag and orientation
            magnitude_f32x4 = vsetq_lane_f32(sqrtf(gx_0*gx_0 + gy_0*gy_0) / cell_wxh, magnitude_f32x4, 0);
            magnitude_f32x4 = vsetq_lane_f32(sqrtf(gx_1*gx_1 + gy_1*gy_1) / cell_wxh, magnitude_f32x4, 1);
            magnitude_f32x4 = vsetq_lane_f32(sqrtf(gx_2*gx_2 + gy_2*gy_2) / cell_wxh, magnitude_f32x4, 2);
            magnitude_f32x4 = vsetq_lane_f32(sqrtf(gx_3*gx_3 + gy_3*gy_3) / cell_wxh, magnitude_f32x4, 3);
            orientation_f32x4 = vsetq_lane_f32(fmod(atan2f(gy_0, gx_0) * pi_3_14, 360), orientation_f32x4, 0);
            orientation_f32x4 = vsetq_lane_f32(fmod(atan2f(gy_1, gx_1) * pi_3_14, 360), orientation_f32x4, 1);
            orientation_f32x4 = vsetq_lane_f32(fmod(atan2f(gy_2, gx_2) * pi_3_14, 360), orientation_f32x4, 2);
            orientation_f32x4 = vsetq_lane_f32(fmod(atan2f(gy_3, gx_3) * pi_3_14, 360), orientation_f32x4, 3);
            
            uint32x4_t lt0 = vcltq_f32(orientation_f32x4, vdupq_n_f32(0.0));
            float32x4_t orientation_f32x4_360 = vaddq_f32(orientation_f32x4, vdupq_n_f32(360.0));
            orientation_f32x4 = vbslq_f32(lt0, orientation_f32x4_360, orientation_f32x4);
            
            //calculating bin.
            int32x4_t bin_s32x4 = vcvtq_s32_f32(vmulq_f32(orientation_f32x4, num_div_360_f32x4));

            int32x4_t cellx_s32x4 = vsetq_lane_s32(i/cell_w, cellx_s32x4, 0);
            cellx_s32x4 = vsetq_lane_s32((i+1)/cell_w, cellx_s32x4, 1);
            cellx_s32x4 = vsetq_lane_s32((i+2)/cell_w, cellx_s32x4, 2);
            cellx_s32x4 = vsetq_lane_s32((i+3)/cell_w, cellx_s32x4, 3);
            int32x4_t magnitudes_index_s32x4 = vaddq_s32(vmulq_s32(celly_s32x4, num_cellw_s32x4), cellx_s32x4);
            int32x4_t bins_index_s32x4 = vaddq_s32(vmulq_s32(magnitudes_index_s32x4, num_orientations_s32x4), bin_s32x4);
            
            void *mag_ptr = (vx_int8 *)magnitudes_data + vgetq_lane_s32(magnitudes_index_s32x4, 0)*2;
            *(vx_int16 *)(mag_ptr) = *(vx_int16 *)(mag_ptr) + vgetq_lane_f32(magnitude_f32x4, 0);
            
            mag_ptr = (vx_int8 *)magnitudes_data + vgetq_lane_s32(magnitudes_index_s32x4, 1)*2;
            *(vx_int16 *)(mag_ptr) = *(vx_int16 *)(mag_ptr) + vgetq_lane_f32(magnitude_f32x4, 1);
            
            mag_ptr = (vx_int8 *)magnitudes_data + vgetq_lane_s32(magnitudes_index_s32x4, 2)*2;
            *(vx_int16 *)(mag_ptr) = *(vx_int16 *)(mag_ptr) + vgetq_lane_f32(magnitude_f32x4, 2);
            
            mag_ptr = (vx_int8 *)magnitudes_data + vgetq_lane_s32(magnitudes_index_s32x4, 3)*2;
            *(vx_int16 *)(mag_ptr) = *(vx_int16 *)(mag_ptr) + vgetq_lane_f32(magnitude_f32x4, 3);

            vx_int8 *bins_ptr = (vx_int8 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 0);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 0);
            
            bins_ptr = (vx_int8 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 1);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 1);
            
            bins_ptr = (vx_int8 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 2);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 2);
            
            bins_ptr = (vx_int8 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 3);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 3);
            
        }
        for (; i < width; i++) {
            vx_int32 x1 = i - 1 < 0 ? 0 : i - 1;
            vx_int32 x2 = i + 1 >= width ? width - 1 : i + 1;
            vx_uint8 *gx1 = vxFormatImagePatchAddress2d(src_base, x1, j, &src_addr);
            vx_uint8 *gx2 = vxFormatImagePatchAddress2d(src_base, x2, j, &src_addr);
            gx = *gx2 - *gx1;

            int y1 = j - 1 < 0 ? 0 : j - 1;
            int y2 = j + 1 >= height ? height - 1 : j + 1;
            vx_uint8 *gy1 = vxFormatImagePatchAddress2d(src_base, i, y1, &src_addr);
            vx_uint8 *gy2 = vxFormatImagePatchAddress2d(src_base, i, y2, &src_addr);
            gy = *gy2 - *gy1;

            //calculating mag and orientation
            magnitude = sqrtf(powf(gx, 2) + powf(gy, 2));// vrsqrteq_f32
            orientation = fmod(atan2f(gy, gx + 0.00000000000001)
                * (180 / 3.14159265), 360);
            if (orientation < 0) {
                orientation += 360;
            }

            //calculating bin.
            bin = (vx_int8)floor(orientation * num_div_360);

            //calculating which cell it belongs to
            vx_int32 cellx = i / cell_w;
            vx_int32 celly = j / cell_h;
            vx_int32 magnitudes_index = celly * num_cellw + cellx;
            vx_int32 bins_index = (celly * num_cellw + cellx) * num_orientations + bin;
            vx_size magnitudes_pos = ComputeGlobalPositionsFromIndex(magnitudes_index, magnitudes_dims, magnitudes_strides, magnitudes_dim_num, &magnitudes_pos);
            vx_size bins_pos = ComputeGlobalPositionsFromIndex(bins_index, bins_dims, bins_strides, bins_dim_num, &bins_pos);
            void *mag_ptr = (vx_int8 *)magnitudes_data + magnitudes_pos;
            void *bins_ptr = (vx_int8 *)bins_data + bins_pos;
            *(vx_int16 *)(mag_ptr) = *(vx_int16 *)(mag_ptr) + magnitude / (cell_w * cell_h);
            *(vx_int8 *)(bins_ptr) = *(vx_int8 *)(bins_ptr) + magnitude / (cell_w * cell_h);
        }
    }

    status |= ReleasePatch(magnitudes, magnitudes_dim_num, magnitudes_dims, magnitudes_strides, &magnitudes_data, VX_WRITE_ONLY);
    status |= ReleasePatch(bins, bins_dim_num, bins_dims, bins_strides, &bins_data, VX_WRITE_ONLY);
    status |= vxUnmapImagePatch(img, map_id_src);
    return status;
}

vx_status vxHogFeatures(vx_image img, void * magnitudes, void * bins, vx_array hog_params, vx_scalar hog_param_size, void * features)
{
    vx_status status = 0;
    void* magnitudes_data = magnitudes;
    void* bins_data = bins;
    void* features_data = features;

    vx_size hog_param_size_t; 
    vx_size hog_params_stride = 0;
    void *hog_params_ptr = NULL;
    vx_map_id hog_params_map_id;
    vx_size hog_params_length;
    vxQueryArray(hog_params, VX_ARRAY_NUMITEMS, &hog_params_length, sizeof(hog_params_length));
    vxMapArrayRange(hog_params, 0, hog_params_length, &hog_params_map_id,
        &hog_params_stride, &hog_params_ptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    vx_hog_t *hog_params_t = (vx_hog_t *)hog_params_ptr;
    status |= vxCopyScalar(hog_param_size, &hog_param_size_t, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    vx_int32 width, height;

    vx_rectangle_t src_rect;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    void *src_base = NULL;
    status |= vxGetValidRegionImage(img, &src_rect);
    vx_map_id map_id_src = 0;
    status |= vxMapImagePatch(img, &src_rect, 0, &map_id_src, &src_addr, &src_base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);

    if (hog_params_t->num_bins > 0 && hog_params_t->num_bins < 360)
    {
        width = src_addr.dim_x;
        height = src_addr.dim_y;

        vx_int32 num_windowsW = width / hog_params_t->window_width;
        vx_int32 num_windowsH = height / hog_params_t->window_height;
        vx_int32 num_blockW = width / hog_params_t->cell_width - 1;
        vx_int32 num_blockH = height / hog_params_t->cell_height - 1;
        vx_int32 num_block = num_blockW * num_blockH;
        vx_int32 n_cellsx = width / hog_params_t->cell_width;
        vx_int32 cells_per_block_w = hog_params_t->block_width / hog_params_t->cell_width;
        vx_int32 cells_per_block_h = hog_params_t->block_height / hog_params_t->cell_height;

        vx_int16 *ptr_src = (vx_int16 *)magnitudes_data;
        vx_int8 *ptr_bins = (vx_int8 *)bins_data;
        vx_int16 *ptr_dst = (vx_int16 *)features_data;

        for (vx_int32 y = 0; y < num_blockH; y++)
        {
            vx_int16 *src_r1 = ptr_src + (y + 0) * n_cellsx;
            vx_int16 *src_r2 = ptr_src + (y + 1) * n_cellsx;

            vx_int8 *bins_r1 = ptr_bins + (y + 0) * n_cellsx * hog_params_t->num_bins;
            vx_int8 *bins_r2 = ptr_bins + (y + 1) * n_cellsx * hog_params_t->num_bins;

            vx_int16 *dst_r1 = ptr_dst + y * (num_blockW + 1) * hog_params_t->num_bins;

            for (vx_int32 x = 0; x < num_blockW * hog_params_t->num_bins; x += hog_params_t->num_bins)
            {
                vx_int32 bidx = x / hog_params_t->num_bins;
                vx_float32 sum = 0.0;
                vx_int16 value1 = src_r1[bidx];
                vx_int16 value2 = src_r1[bidx + 1];
                vx_int16 value3 = src_r2[bidx];
                vx_int16 value4 = src_r2[bidx + 1];
                sum += value1 * value1;
                sum += value2 * value2;
                sum += value3 * value3;
                sum += value4 * value4;
                vx_float32 scale = 1.f / sqrtf(sum + 0.00000000000001);

                vx_int8 *bins1 = bins_r1 + (x + 0);
                vx_int8 *bins2 = bins_r1 + (x + 1);

                vx_int8 *bins3 = bins_r2 + (x + 0);
                vx_int8 *bins4 = bins_r2 + (x + 1);

                vx_int16 *dst = dst_r1 + x;
                for (int k = 0; k < hog_params_t->num_bins; k++)
                {
                    vx_float32 hist = 0.0;

                    hist += min(bins1[k] * scale, hog_params_t->threshold);
                    hist += min(bins2[k] * scale, hog_params_t->threshold);
                    hist += min(bins3[k] * scale, hog_params_t->threshold);
                    hist += min(bins4[k] * scale, hog_params_t->threshold);

                    dst[k] += hist;
                }
            }
        }
    }

    status |= vxUnmapImagePatch(img, map_id_src);
    status |= vxUnmapArrayRange(hog_params, hog_params_map_id);
    
    return status;
}
