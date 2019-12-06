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

#ifndef min
#define min(a,b) (a<b?a:b)
#endif

void HogCells_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_int32 *cell_w = (vx_int32 *)parameters[1];
    vx_int32 *cell_h = (vx_int32 *)parameters[2];
    vx_int32 *num_orientations = (vx_int32 *)parameters[3];
    void* magnitudes_data = parameters[4];
    void* bins_data = parameters[5];
    vx_float32 gx;
    vx_float32 gy;
    vx_float32 orientation;
    vx_float32 magnitude;
    vx_int8 bin;
    float num_div_360 = (float)(*num_orientations) / 360.0f;
    vx_int32 num_cellw = (vx_int32)floor(((vx_float64)in->image.width) / ((vx_float64)(*cell_w)));    
    vx_uint32 low_height = in->tile_y;
    vx_uint32 height = in->tile_y + in->tile_block.height;    
    vx_uint32 low_width = in->tile_x;
    vx_uint32 width = in->tile_x + in->tile_block.width;
    
    vx_float32 gx_0, gx_1, gx_2, gx_3;
    vx_float32 gy_0, gy_1, gy_2, gy_3;
    float32x4_t magnitude_f32x4;
    float32x4_t orientation_f32x4;
    float32x4_t fv_0_5_32x4 = vdupq_n_f32(0.5f);
    float32x4_t num_div_360_f32x4 = vdupq_n_f32(num_div_360);
    int32x4_t bin_s32x4;
    vx_int32 cell_wxh = (*cell_w)*(*cell_h);
    int32x4_t num_orientations_s32x4 = vdupq_n_s32((*num_orientations));
    int32x4_t num_cellw_s32x4 = vdupq_n_s32(num_cellw);
    float32_t pi_3_14 = 180 / 3.14159265;
    
    for (vx_int32 j = low_height; j < height; j++) 
    {        
        int32x4_t celly_s32x4 = vdupq_n_s32(j/(*cell_h));
        vx_int32 y1 = j - 1 < 0 ? 0 : j - 1;
        vx_int32 y2 = j + 1 >= in->image.height ? in->image.height - 1 : j + 1;
        vx_uint8 *src_base_y = (vx_uint8 *)in->base[0] + j*in->addr[0].stride_y;
        vx_uint8 *src_base_y_y1 = (vx_uint8 *)in->base[0] + y1*in->addr[0].stride_y;
        vx_uint8 *src_base_y_y2 = (vx_uint8 *)in->base[0] + y2*in->addr[0].stride_y;        
        for (int i = low_width; i < width; i+=4) 
        {            
            vx_int32 x1 = i - 1 < 0 ? 0 : i - 1;
            vx_int32 x2 = i + 1 >= in->image.width ? in->image.width - 1 : i + 1;
            gx_0 = *(src_base_y + x2) - *(src_base_y + x1);                     
            x1 = i < 0 ? 0 : i;
            x2 = i + 2 >= in->image.width ? in->image.width - 1 : i+2;
            gx_1 = *(src_base_y + x2) - *(src_base_y + x1);            
            x1 = i + 1 < 0 ? 0 : i + 1;
            x2 = i+3 >= in->image.width ? in->image.width - 1 : i+3;
            gx_2 = *(src_base_y + x2) - *(src_base_y + x1);             
            x1 = i+2 < 0 ? 0 : i+2;
            x2 = i+4 >= in->image.width ? in->image.width - 1 : i+4;
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

            int32x4_t cellx_s32x4 = vsetq_lane_s32(i/(*cell_w), cellx_s32x4, 0);
            cellx_s32x4 = vsetq_lane_s32((i+1)/(*cell_w), cellx_s32x4, 1);
            cellx_s32x4 = vsetq_lane_s32((i+2)/(*cell_w), cellx_s32x4, 2);
            cellx_s32x4 = vsetq_lane_s32((i+3)/(*cell_w), cellx_s32x4, 3);
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
            vx_int16 *bins_ptr = (vx_int16 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 0);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 0);            
            bins_ptr = (vx_int16 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 1);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 1);            
            bins_ptr = (vx_int16 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 2);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 2);            
            bins_ptr = (vx_int16 *)bins_data + vgetq_lane_s32(bins_index_s32x4, 3);
            *bins_ptr = *bins_ptr + vgetq_lane_f32(magnitude_f32x4, 3);            
        }
    }
}

#define HOGCELLS_SCALING(low_y, low_x, high_y, high_x, in_tile_x)\
    for (int j = low_y; j < high_y; j++) {\
        for (int i = low_x; i < high_x; i++) {\
            int x1 = i - 1 < 0 ? 0 : i - 1;\
            int x2 = i + 1 >= high_x ? high_x - 1 : i + 1;\
            vx_uint8 *gx1 = (vx_uint8 *)in->base[0] + in_tile_x + j * in->addr[0].stride_y + x1 * in->addr[0].stride_x;\
            vx_uint8 *gx2 = (vx_uint8 *)in->base[0] + in_tile_x + j * in->addr[0].stride_y + x2 * in->addr[0].stride_x;\
            gx = *gx2 - *gx1;\
            int y1 = j - 1 < 0 ? 0 : j - 1;\
            int y2 = j + 1 >= high_y ? high_y - 1 : j + 1;\
            vx_uint8 *gy1 = (vx_uint8 *)in->base[0] + in_tile_x + y1 * in->addr[0].stride_y + i * in->addr[0].stride_x;\
            vx_uint8 *gy2 = (vx_uint8 *)in->base[0] + in_tile_x + y2 * in->addr[0].stride_y + i * in->addr[0].stride_x;\
            gy = *gy2 - *gy1;\
            magnitude = sqrtf(powf(gx, 2) + powf(gy, 2));\
            orientation = fmod(atan2f(gy, gx + 0.00000000000001)\
                * (180 / 3.14159265), 360);\
            if (orientation < 0) {\
                orientation += 360;\
            }\
            bin = (vx_int8)floor(orientation * num_div_360);\
            vx_int32 cellx = i / (*cell_w);\
            vx_int32 celly = j / (*cell_h);\
            vx_int32 magnitudes_index = celly * num_cellw + cellx;\
            vx_int32 bins_index = (celly * num_cellw + cellx) * (*num_orientations) + bin;\
            vx_size magnitudes_pos = 2 * magnitudes_index;\
            vx_size bins_pos = bins_index;\
            void *mag_ptr = (vx_int8 *)magnitudes_data + magnitudes_pos;\
            void *bins_ptr = (vx_int8 *)bins_data + bins_pos;\
            *(vx_int16 *)(mag_ptr) = *(vx_int16 *)(mag_ptr) + magnitude / ((*cell_w) * (*cell_h));\
            *(vx_int16 *)(bins_ptr) = *(vx_int16 *)(bins_ptr) + magnitude / ((*cell_w) * (*cell_h));\
        }\
    }\

void HogCells_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 y, x;
    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    vx_int32 *cell_w = (vx_int32 *)parameters[1];
    vx_int32 *cell_h = (vx_int32 *)parameters[2];
    vx_int32 *num_orientations = (vx_int32 *)parameters[3];
    void* magnitudes_data = parameters[4];
    void* bins_data = parameters[5];
    vx_float32 gx;
    vx_float32 gy;
    vx_float32 orientation;
    vx_float32 magnitude;
    vx_int8 bin;

    float num_div_360 = (float)(*num_orientations) / 360.0f;
    vx_int32 num_cellw = (vx_int32)floor(((vx_float64)in->image.width) / ((vx_float64)(*cell_w)));
    vx_uint32 ty = in->tile_y;
    vx_uint32 tx = in->tile_x;
    if (ty == 0 && tx == 0)
    {
        HOGCELLS_SCALING(0, 0, vxTileHeight(in, 0), vxTileWidth(in, 0), in->tile_x)     
    }
    else
    {
        HOGCELLS_SCALING(0, tx, ty, vxTileWidth(in, 0), in->tile_x)
        HOGCELLS_SCALING(ty, 0, vxTileHeight(in, 0), vxTileWidth(in, 0), 0)
    }
}

void HogFeatures_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_int32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    void *magnitudes_data = parameters[1];
    void * bins_data = parameters[2];
    vx_tile_array_t *hog_params = (vx_tile_array_t *)parameters[3];
    void * features_data = parameters[5];

    vx_uint32 high_y = in->tile_y + in->tile_block.height;

    vx_uint32 high_x = in->tile_x + in->tile_block.width;

    vx_int32 width, height;

    vx_hog_t *hog_params_t = (vx_hog_t *)hog_params->ptr;

    if (hog_params_t->num_bins > 0 && hog_params_t->num_bins < 360)
    {
        width = high_x;
        height = high_y;
        vx_int32 num_blockW = width / hog_params_t->cell_width - 1;
        vx_int32 num_blockH = height / hog_params_t->cell_height - 1;
        vx_int32 n_cellsx = width / hog_params_t->cell_width;
        vx_int32 cells_per_block_w = hog_params_t->block_width / hog_params_t->cell_width;
        vx_int32 cells_per_block_h = hog_params_t->block_height / hog_params_t->cell_height;

        vx_int16 *ptr_src = (vx_int16 *)magnitudes_data;
        vx_int8 *ptr_bins = (vx_int8 *)bins_data;
        vx_int16 *ptr_dst = (vx_int16 *)features_data;
        vx_int32 num_bins_s32 = hog_params_t->num_bins;            
        vx_int32 roiw4 = num_blockW * num_bins_s32 >= 3*num_bins_s32 ? num_blockW * num_bins_s32 : 0;

        for (y = 0; y < num_blockH; y++)
        {
            vx_int16 *src_r1 = ptr_src + (y + 0) * n_cellsx;
            vx_int16 *src_r2 = ptr_src + (y + 1) * n_cellsx;
            vx_int8 *bins_r1 = ptr_bins + (y + 0) * n_cellsx * hog_params_t->num_bins;
            vx_int8 *bins_r2 = ptr_bins + (y + 1) * n_cellsx * hog_params_t->num_bins;
            vx_int16 *dst_r1 = ptr_dst + y * (num_blockW + 1) * hog_params_t->num_bins;
            for (x = 0; x < roiw4; x += 4*num_bins_s32)
            {
                int32x4_t bidx_s32x4;
                bidx_s32x4 = vsetq_lane_s32(x / num_bins_s32, bidx_s32x4, 0);
                bidx_s32x4 = vsetq_lane_s32((x + num_bins_s32) / num_bins_s32, bidx_s32x4, 1);
                bidx_s32x4 = vsetq_lane_s32((x + 2 * num_bins_s32) / num_bins_s32, bidx_s32x4, 2);
                bidx_s32x4 = vsetq_lane_s32((x + 3 * num_bins_s32) / num_bins_s32, bidx_s32x4, 3);

                float32x4_t sum_f32x4;
                int16x4_t value1_s16x4;
                int16x4_t value2_s16x4;
                int16x4_t value3_s16x4;
                int16x4_t value4_s16x4;                
                value1_s16x4 = vset_lane_s16(src_r1[vgetq_lane_s32(bidx_s32x4, 0)], value1_s16x4, 0);
                value1_s16x4 = vset_lane_s16(src_r1[vgetq_lane_s32(bidx_s32x4, 1)], value1_s16x4, 1);
                value1_s16x4 = vset_lane_s16(src_r1[vgetq_lane_s32(bidx_s32x4, 2)], value1_s16x4, 2);
           
                value2_s16x4 = vset_lane_s16(src_r1[vgetq_lane_s32(bidx_s32x4, 0) + 1], value2_s16x4, 0);
                value2_s16x4 = vset_lane_s16(src_r1[vgetq_lane_s32(bidx_s32x4, 1) + 1], value2_s16x4, 1);
                value2_s16x4 = vset_lane_s16(src_r1[vgetq_lane_s32(bidx_s32x4, 2) + 1], value2_s16x4, 2);
             
                value3_s16x4 = vset_lane_s16(src_r2[vgetq_lane_s32(bidx_s32x4, 0)], value3_s16x4, 0);
                value3_s16x4 = vset_lane_s16(src_r2[vgetq_lane_s32(bidx_s32x4, 1)], value3_s16x4, 1);
                value3_s16x4 = vset_lane_s16(src_r2[vgetq_lane_s32(bidx_s32x4, 2)], value3_s16x4, 2);
              
                value4_s16x4 = vset_lane_s16(src_r2[vgetq_lane_s32(bidx_s32x4, 0) + 1], value4_s16x4, 0);
                value4_s16x4 = vset_lane_s16(src_r2[vgetq_lane_s32(bidx_s32x4, 1) + 1], value4_s16x4, 1);
                value4_s16x4 = vset_lane_s16(src_r2[vgetq_lane_s32(bidx_s32x4, 2) + 1], value4_s16x4, 2);
 
                sum_f32x4 = vcvtq_f32_s32(vmovl_s16(vadd_s16(vadd_s16(vmul_s16(value1_s16x4, value1_s16x4), vmul_s16(value2_s16x4, value2_s16x4)),
                    vadd_s16(vmul_s16(value3_s16x4, value3_s16x4), vmul_s16(value4_s16x4, value4_s16x4)))));

                vx_float32 scale = 1.f / sqrtf(vgetq_lane_f32(sum_f32x4, 0) + 0.00000000000001f);
                vx_int8 *bins1 = bins_r1 + (x + 0);
                vx_int8 *bins2 = bins_r1 + (x + 1);
                vx_int8 *bins3 = bins_r2 + (x + 0);
                vx_int8 *bins4 = bins_r2 + (x + 1);
                vx_int16 *dst = dst_r1 + x;
                for (int k = 0; k < num_bins_s32; k++)
                {
                    vx_float32 hist = 0.0;
                    hist += min((vx_int16)bins1[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins2[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins3[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins4[k] * scale, hog_params_t->threshold);
                    dst[k] += hist;
                }
                
                scale = 1.f / sqrtf(vgetq_lane_f32(sum_f32x4, 1) + 0.00000000000001f);
                bins1 = bins_r1 + (x + 0 + num_bins_s32);
                bins2 = bins_r1 + (x + 1 + num_bins_s32);
                bins3 = bins_r2 + (x + 0 + num_bins_s32);
                bins4 = bins_r2 + (x + 1 + num_bins_s32);
                dst = dst_r1 + x + num_bins_s32;
                for (int k = 0; k < num_bins_s32; k++)
                {
                    vx_float32 hist = 0.0;
                    hist += min((vx_int16)bins1[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins2[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins3[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins4[k] * scale, hog_params_t->threshold);
                    dst[k] += hist;
                }
                
                scale = 1.f / sqrtf(vgetq_lane_f32(sum_f32x4, 2) + 0.00000000000001f);
                bins1 = bins_r1 + (x + 0 + 2*num_bins_s32);
                bins2 = bins_r1 + (x + 1 + 2*num_bins_s32);
                bins3 = bins_r2 + (x + 0 + 2*num_bins_s32);
                bins4 = bins_r2 + (x + 1 + 2*num_bins_s32);
                dst = dst_r1 + x + 2*num_bins_s32;
                for (int k = 0; k < num_bins_s32; k++)
                {
                    vx_float32 hist = 0.0;
                    hist += min((vx_int16)bins1[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins2[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins3[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins4[k] * scale, hog_params_t->threshold);
                    dst[k] += hist;
                }
                
                scale = 1.f / sqrtf(vgetq_lane_f32(sum_f32x4, 3) + 0.00000000000001f);
                bins1 = bins_r1 + (x + 0 + 3*num_bins_s32);
                bins2 = bins_r1 + (x + 1 + 3*num_bins_s32);
                bins3 = bins_r2 + (x + 0 + 3*num_bins_s32);
                bins4 = bins_r2 + (x + 1 + 3*num_bins_s32);
                dst = dst_r1 + x + 3*num_bins_s32;
                for (int k = 0; k < num_bins_s32; k++)
                {
                    vx_float32 hist = 0.0;
                    hist += min((vx_int16)bins1[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins2[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins3[k] * scale, hog_params_t->threshold);
                    hist += min((vx_int16)bins4[k] * scale, hog_params_t->threshold);
                    dst[k] += hist;
                }
            }
        }
    }
}


#define HOGFEATURES(low_y, high_y, low_x)                                                                                              \
    for (vx_int32 blkH = 0; blkH < num_blockH; blkH++)                                                                                 \
    {                                                                                                                                  \
        for (vx_int32 blkW = 0; blkW < num_blockW; blkW++)                                                                             \
        {                                                                                                                              \
            vx_float32 sum = 0;                                                                                                        \
            for (vx_int32 y = 0; y < cells_per_block_h; y++)                                                                           \
            {                                                                                                                          \
                for (vx_int32 x = 0; x < cells_per_block_w; x++)                                                                       \
                {                                                                                                                      \
                    vx_int32 index = (blkH + y)*n_cellsx + (blkW + x);                                                                 \
                    void *mag_ptr = (vx_int8 *)magnitudes_data + index;                                                                \
                    sum += (*(vx_int16 *)mag_ptr) * (*(vx_int16 *)mag_ptr);                                                            \
                }                                                                                                                      \
            }                                                                                                                          \
            sum = sqrtf(sum + 0.00000000000001f);                                                                                      \
            for (vx_int32 y = 0; y < cells_per_block_h; y++)                                                                           \
            {                                                                                                                          \
                for (vx_int32 x = 0; x < cells_per_block_w; x++)                                                                       \
                {                                                                                                                      \
                    for (vx_int32 k = 0; k < hog_params_t->num_bins; k++)                                                              \
                    {                                                                                                                  \
                        vx_int32 bins_index = (blkH + y)*n_cellsx * hog_params_t->num_bins + (blkW + x)*hog_params_t->num_bins + k;    \
                        vx_int32 block_index = blkH * num_blockW * hog_params_t->num_bins + blkW * hog_params_t->num_bins + k;         \
                        float hist = min((vx_int16)(*((vx_int8 *)bins_data + bins_index)) / sum, hog_params_t->threshold);             \
                        void *features_ptr = (vx_int8 *)features_data + block_index;                                                   \
                        *(vx_int16 *)features_ptr = *(vx_int16 *)features_ptr + hist;                                                  \
                    }                                                                                                                  \
                }                                                                                                                      \
            }                                                                                                                          \
        }                                                                                                                              \
    }

void HogFeatures_image_tiling_flexible(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x = 0, y = 0;

    vx_tile_ex_t *in = (vx_tile_ex_t *)parameters[0];
    void *magnitudes_data = parameters[1];
    void *bins_data = parameters[2];
    vx_tile_array_t *hog_params = (vx_tile_array_t *)parameters[3];
    void * features_data = parameters[5];

    vx_uint32 low_y = in->tile_y;
    vx_uint32 high_y = vxTileHeight(in, 0);

    vx_uint32 low_x = in->tile_x;
    vx_uint32 high_x = vxTileWidth(in, 0);

    vx_int32 width = high_x, height = high_y;

    vx_hog_t *hog_params_t = (vx_hog_t *)hog_params->ptr;

    vx_int32 num_blockW = width / hog_params_t->cell_width - 1;
    vx_int32 num_blockH = height / hog_params_t->cell_height - 1;
    vx_int32 n_cellsx = width / hog_params_t->cell_width;
    vx_int32 cells_per_block_w = hog_params_t->block_width / hog_params_t->cell_width;
    vx_int32 cells_per_block_h = hog_params_t->block_height / hog_params_t->cell_height;

    if (hog_params_t->num_bins > 0 && hog_params_t->num_bins < 360)
    {
        if (low_y == 0 && low_x == 0)
        {
            HOGFEATURES(low_y, high_y, low_x)
        }
        else
        {
            HOGFEATURES(0, low_y, low_x)
            HOGFEATURES(low_y, high_y, 0)
        }
    }
}
