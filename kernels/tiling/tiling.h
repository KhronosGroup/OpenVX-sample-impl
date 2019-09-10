/*

* Copyright (c) 2011-2017 The Khronos Group Inc.
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

#include <VX/vx_khr_tiling.h>

void box3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void box3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Phase_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Phase_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void And_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void And_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Or_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Or_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Xor_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Xor_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Not_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Not_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Threshold_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Threshold_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void ConvertColor_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void ConvertColor_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Multiply_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Multiply_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void NonLinearFilter_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void NonLinearFilter_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Magnitude_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Magnitude_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Erode3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Erode3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Dilate3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Dilate3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Median3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Median3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Sobel3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Sobel3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Max_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Max_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Min_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Min_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Gaussian3x3_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Gaussian3x3_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Addition_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Addition_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void Subtraction_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void Subtraction_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void ConvertDepth_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void ConvertDepth_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void WarpAffine_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void WarpAffine_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void WarpPerspective_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void WarpPerspective_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);

void WeightedAverage_image_tiling_fast(void * parameters[], void * tile_memory, vx_size tile_memory_size);
void WeightedAverage_image_tiling_flexible(void * parameters[], void * tile_memory, vx_size tile_memory_size);
