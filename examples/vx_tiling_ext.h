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

#ifndef _VX_EXAMPLE_TILING_H_
#define _VX_EXAMPLE_TILING_H_

/*! \brief The list of example tiling kernels */
enum vx_tiling_kernels_e {
    /*! \brief a re-implementation of a gaussian blur */
    VX_KERNEL_GAUSSIAN_3x3_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x100,
    /*! \brief a re-implementation of a generic box filter of any size */
    VX_KERNEL_BOX_MxN_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x101,
    /*! \brief a re-implementation of a add kernel */
    VX_KERNEL_ADD_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x102,
    /*! \brief an alpha multiply kernel */
    VX_KERNEL_ALPHA_TILING = VX_KERNEL_BASE(VX_ID_KHRONOS, VX_LIBRARY_KHR_BASE) + 0x103,
};

#ifdef _VX_TILING_EXT_INTERNAL_
void add_image_tiling(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void alpha_image_tiling(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void box_image_tiling(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
void gaussian_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT], void * VX_RESTRICT tile_memory, vx_size tile_memory_size);
#endif

#endif
