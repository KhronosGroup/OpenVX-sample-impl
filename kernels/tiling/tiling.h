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

enum vx_tiling_kernels_e
{
    VX_KERNEL_BOX_3x3_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x100,
    VX_KERNEL_PHASE_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x101,
	VX_KERNEL_AND_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x102,
    VX_KERNEL_OR_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x103,
    VX_KERNEL_XOR_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x104,
    VX_KERNEL_NOT_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x105,
};

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
