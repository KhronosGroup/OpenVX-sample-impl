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

#ifndef _VX_EXTRAS_K_H_
#define _VX_EXTRAS_K_H_

#include <VX/vx.h>
#include <VX/vx_helper.h>

#ifdef __cplusplus
extern "C" {
#endif

vx_status ownEuclideanNonMaxSuppressionHarris(vx_image src, vx_scalar thr, vx_scalar rad, vx_image dst);
vx_status ownNonMaxSuppression(vx_image i_mag, vx_image i_ang, vx_image i_edge, vx_border_t* bordermode);
vx_status ownLaplacian3x3(vx_image src, vx_image dst, vx_border_t* bordermode);

#ifdef __cplusplus
}
#endif

#endif  // !_VX_EXTRAS_K_H_
