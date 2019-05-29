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

#ifndef _VX_VENUM_H_
#define _VX_VENUM_H_

#include <VX/vx.h>
#include <VX/vx_helper.h>
/* TODO: remove vx_compatibility.h after transition period */
#include <VX/vx_compatibility.h>
#include <VX/vx_khr_nn.h>

#include <math.h>
#include <stdbool.h>

//TODO(ruv): temp, remove me
#define USE_NEW_SAMPLE_CONV_KERNEL_FOR_Q78
//#define USE_NEW_SAMPLE_FULLYCONNECTED_KERNEL_FOR_Q78
#define USE_NEW_SAMPLE_POOL_KERNEL_FOR_Q78
#define USE_NEW_SAMPLE_SOFTMAX_KERNEL_FOR_Q78
//#define USE_NEW_SAMPLE_NORM_KERNEL_FOR_Q78
//#define USE_NEW_SAMPLE_ACTIVATION_KERNEL_FOR_Q78
#define USE_NEW_SAMPLE_ROIPOOL_KERNEL
//#define USE_NEW_SAMPLE_DECONV_KERNEL_FOR_Q78

//TODO(ruv): temp, remove me
#define HACK_FOR_LACK_OF_INNER_NODE_OUTPUT_MEM_ALLOC

/*! \brief The largest convolution matrix the specification requires support for is 15x15.
 */
#define VENUM_MAX_CONVOLUTION_DIM (15)

/*! \brief The largest nonlinear filter matrix the specification requires support for is 9x9.
*/
#define VENUM_MAX_NONLINEAR_DIM (9)


#ifdef __cplusplus
extern "C" {
#endif

vx_status vxAbsDiff(vx_image in1, vx_image in2, vx_image output);

#ifdef __cplusplus
}
#endif

#endif
