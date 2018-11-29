/*

 * Copyright (c) 2016-2017 The Khronos Group Inc.
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

#ifndef UTILS_TENSOR_UTILS_H_
#define UTILS_TENSOR_UTILS_H_
#include "VX/vx_types.h"
#define MAX_NUM_OF_DIMENSIONS 6

vx_size ComputeGlobalPositionsFromIndex(vx_size index, const vx_size * dimensions,
		const vx_size * stride, vx_size number_of_dimensions, vx_size * pos);
vx_size ComputeNumberOfElements (const vx_size * dimensions, vx_size number_of_dimensions);

vx_status AllocatePatch (vx_tensor tensor, vx_size* dims_num, vx_size* dimensions, vx_size* stride, void** buffer_ptr, vx_enum usage);

vx_status ReleasePatch (vx_tensor tensor, vx_size dims_num, const vx_size* dimensions, const vx_size* stride, void** buffer_ptr, vx_enum usage) ;

#endif /* UTILS_TENSOR_UTILS_H_ */
