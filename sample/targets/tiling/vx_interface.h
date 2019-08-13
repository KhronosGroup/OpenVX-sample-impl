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

#include <VX/vx_helper.h>
#include <VX/vx_khr_tiling.h>

 // tag::publish_support[]
typedef struct _vx_tiling_kernel_t {
    /*! kernel name */
    vx_char name[VX_MAX_KERNEL_NAME];
    /*! kernel enum */
    vx_enum enumeration;
    /*! tiling flexible function pointer */
    vx_tiling_kernel_f flexible_function;
    /*! tiling fast function pointer */
    vx_tiling_kernel_f fast_function;
    /*! number of parameters */
    vx_uint32 num_params;
    /*! set of parameters */
    vx_param_description_t parameters[10];
    /*! input validator */
    //vx_kernel_input_validate_f input_validator;
    void *input_validator;
    /*! output validator */
    //vx_kernel_output_validate_f output_validator;
    void *output_validator;
    /*! block size */
    vx_tile_block_size_t block;
    /*! neighborhood around block */
    vx_neighborhood_size_t nbhd;
    /*! border information. */
    vx_border_t border;
} vx_tiling_kernel_t;

extern vx_tiling_kernel_t box_3x3_kernels;
extern vx_tiling_kernel_t phase_kernel;
extern vx_tiling_kernel_t And_kernel;
extern vx_tiling_kernel_t Or_kernel;
extern vx_tiling_kernel_t Xor_kernel;
extern vx_tiling_kernel_t Not_kernel;

#endif

