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

#include <c_model.h>
#include <stdio.h>
#include <stdlib.h>
#include "tensor_utils.h"

#ifndef min
#define min(a,b) (a<b?a:b)
#endif 
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
    status |= vxAccessImagePatch(img, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    width = src_addr.dim_x;
    height = src_addr.dim_y;

    void* magnitudes_data = NULL;
    void* bins_data = NULL;
    vx_size magnitudes_dim_num = 0, magnitudes_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, magnitudes_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };
    vx_size bins_dim_num = 0, bins_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, bins_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };

    status |= AllocatePatch(magnitudes, &magnitudes_dim_num, magnitudes_dims, magnitudes_strides, &magnitudes_data, VX_WRITE_ONLY);
    status |= AllocatePatch(bins, &bins_dim_num, bins_dims, bins_strides, &bins_data, VX_WRITE_ONLY);

    float num_div_360 = (float)num_orientations / 360.0f;
    vx_int32 num_cellw = (vx_int32)floor(((vx_float64)width) / ((vx_float64)cell_w));
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {

            //calculating gx and gy
            int x1 = i - 1 < 0 ? 0 : i - 1;
            int x2 = i + 1 >= width ? width - 1 : i + 1;
            vx_uint8 *gx1 = vxFormatImagePatchAddress2d(src_base, x1, j, &src_addr);
            vx_uint8 *gx2 = vxFormatImagePatchAddress2d(src_base, x2, j, &src_addr);
            gx = (vx_float32)(*gx2 - *gx1);

            int y1 = j - 1 < 0 ? 0 : j - 1;
            int y2 = j + 1 >= height ? height - 1 : j + 1;
            vx_uint8 *gy1 = vxFormatImagePatchAddress2d(src_base, i, y1, &src_addr);
            vx_uint8 *gy2 = vxFormatImagePatchAddress2d(src_base, i, y2, &src_addr);
            gy = (vx_float32)(*gy2 - *gy1);

            //calculating mag and orientation
            magnitude = sqrtf(powf(gx, 2) + powf(gy, 2));
            orientation = (float)fmod((double)atan2f(gy, gx + 0.00000000000001f)
                * (180.0f / 3.14159265f), 360);
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
            *(vx_int16 *)(mag_ptr) = *(vx_int16 *)(mag_ptr) + (vx_int16)(magnitude / (float)(cell_w * cell_h));
            *(vx_int8 *)(bins_ptr) = *(vx_int8 *)(bins_ptr) + (vx_int8)(magnitude / (float)(cell_w * cell_h));
        }
    }

    status |= ReleasePatch(magnitudes, magnitudes_dim_num, magnitudes_dims, magnitudes_strides, &magnitudes_data, VX_WRITE_ONLY);
    status |= ReleasePatch(bins, bins_dim_num, bins_dims, bins_strides, &bins_data, VX_WRITE_ONLY);
    status |= vxCommitImagePatch(img, &rect, 0, &src_addr, src_base);

    return status;
}

vx_status vxHogFeatures(vx_image img, vx_tensor magnitudes, vx_tensor bins, vx_array hog_params, vx_scalar hog_param_size, vx_tensor features)
{
    vx_status status;
    void* magnitudes_data = NULL;
    void* bins_data = NULL;
    void* features_data = NULL;

    vx_size hog_param_size_t;
    vx_size magnitudes_dim_num = 0, magnitudes_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, magnitudes_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };
    vx_size bins_dim_num = 0, bins_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, bins_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };
    vx_size features_dim_num = 0, features_dims[MAX_NUM_OF_DIMENSIONS] = { 0 }, features_strides[MAX_NUM_OF_DIMENSIONS] = { 0 };

    status = AllocatePatch(magnitudes, &magnitudes_dim_num, magnitudes_dims, magnitudes_strides, &magnitudes_data, VX_READ_ONLY);
    status |= AllocatePatch(bins, &bins_dim_num, bins_dims, bins_strides, &bins_data, VX_READ_ONLY);
    status |= AllocatePatch(features, &features_dim_num, features_dims, features_strides, &features_data, VX_WRITE_ONLY);

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
    status |= vxAccessImagePatch(img, &src_rect, 0, &src_addr, (void **)&src_base, VX_READ_AND_WRITE);
    width = src_addr.dim_x;
    height = src_addr.dim_y;

    vx_int32 num_blockW = width / hog_params_t->cell_width - 1;
    vx_int32 num_blockH = height / hog_params_t->cell_height - 1;
    vx_int32 n_cellsx = width / hog_params_t->cell_width;
    vx_int32 cells_per_block_w = hog_params_t->block_width / hog_params_t->cell_width;
    vx_int32 cells_per_block_h = hog_params_t->block_height / hog_params_t->cell_height;

    for (vx_int32 blkH = 0; blkH < num_blockH; blkH++)
    {
        for (vx_int32 blkW = 0; blkW < num_blockW; blkW++)
        {
            vx_float32 sum = 0;
            for (vx_int32 y = 0; y < cells_per_block_h; y++)
            {
                for (vx_int32 x = 0; x < cells_per_block_w; x++)
                {
                    vx_int32 index = (blkH + y)*n_cellsx + (blkW + x);
                    void *mag_ptr = (vx_int8 *)magnitudes_data + index;
                    sum += (*(vx_int16 *)mag_ptr) * (*(vx_int16 *)mag_ptr);
                }
            }
            sum = sqrtf(sum + 0.00000000000001f);
            for (vx_int32 y = 0; y < cells_per_block_h; y++)
            {
                for (vx_int32 x = 0; x < cells_per_block_w; x++)
                {
                    for (vx_int32 k = 0; k < hog_params_t->num_bins; k++)
                    {
                        vx_int32 bins_index = (blkH + y)*n_cellsx * hog_params_t->num_bins + (blkW + x)*hog_params_t->num_bins + k;
                        vx_int32 block_index = blkH * num_blockW * hog_params_t->num_bins + blkW * hog_params_t->num_bins + k;
                        float hist = min((vx_int8)(*((vx_int8 *)bins_data + bins_index)) / sum, hog_params_t->threshold);
                        void *features_ptr = (vx_int8 *)features_data + block_index;
                        *(vx_int16 *)features_ptr = *(vx_int16 *)features_ptr + (vx_int16)hist;
                    }
                }
            }
        }
    }
    status |= vxCommitImagePatch(img, &src_rect, 0, &src_addr, src_base);
    status |= vxUnmapArrayRange(hog_params, hog_params_map_id);
    status |= ReleasePatch(magnitudes, magnitudes_dim_num, magnitudes_dims, magnitudes_strides, &magnitudes_data, VX_READ_ONLY);
    status |= ReleasePatch(bins, bins_dim_num, bins_dims, bins_strides, &bins_data, VX_READ_ONLY);
    status |= ReleasePatch(features, features_dim_num, features_dims, features_strides, &features_data, VX_WRITE_ONLY);
    return status;
}
