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

/*!
 * \file
 * \brief The Extra Extensions Module Header.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#ifndef _VX_EXTRAS_MODULE_H_
#define _VX_EXTRAS_MODULE_H_

#ifdef  __cplusplus
extern "C" {
#endif

extern vx_kernel_description_t edge_trace_kernel;
extern vx_kernel_description_t euclidean_nonmaxsuppression_harris_kernel;
extern vx_kernel_description_t harris_score_kernel;
extern vx_kernel_description_t laplacian3x3_kernel;
extern vx_kernel_description_t image_lister_kernel;
extern vx_kernel_description_t nonmaxsuppression_kernel;
extern vx_kernel_description_t norm_kernel;
extern vx_kernel_description_t scharr3x3_kernel;
extern vx_kernel_description_t sobelMxN_kernel;

#ifdef  __cplusplus
}
#endif

#endif  /* _VX_EXTRAS_MODULE_H_ */

