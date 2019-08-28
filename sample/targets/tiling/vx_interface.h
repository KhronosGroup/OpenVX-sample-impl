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

#ifndef _OPENVX_INTERFACE_H_
#define _OPENVX_INTERFACE_H_

#include <VX/vx_khr_tiling.h>

extern vx_tiling_kernel_t box_3x3_kernels;
extern vx_tiling_kernel_t phase_kernel;
extern vx_tiling_kernel_t And_kernel;
extern vx_tiling_kernel_t Or_kernel;
extern vx_tiling_kernel_t Xor_kernel;
extern vx_tiling_kernel_t Not_kernel;
extern vx_tiling_kernel_t threshold_kernel;
extern vx_tiling_kernel_t colorconvert_kernel;
extern vx_tiling_kernel_t Multiply_kernel;
extern vx_tiling_kernel_t nonlinearfilter_kernel;
extern vx_tiling_kernel_t Magnitude_kernel;
extern vx_tiling_kernel_t erode3x3_kernel;
extern vx_tiling_kernel_t dilate3x3_kernel;
extern vx_tiling_kernel_t median3x3_kernel;

#endif

