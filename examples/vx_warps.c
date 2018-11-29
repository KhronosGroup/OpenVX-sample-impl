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

#include <VX/vx.h>
#include <VX/vxu.h>

vx_status vx_example_warp_affine(vx_context context, vx_image src, vx_image dst)
{
    vx_float32 a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f, e = 0.0f , f = 0.0f;
    //! [warp affine]
    // x0 = a x + b y + c;
    // y0 = d x + e y + f;
    vx_float32 mat[3][2] = {
        {a, d}, // 'x' coefficients
        {b, e}, // 'y' coefficients
        {c, f}, // 'offsets'
    };
    vx_matrix matrix = vxCreateMatrix(context, VX_TYPE_FLOAT32, 2, 3);
    vxCopyMatrix(matrix, mat, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    //! [warp affine]
    return vxuWarpAffine(context, src, matrix, VX_INTERPOLATION_NEAREST_NEIGHBOR, dst);
}

vx_status vx_example_warp_perspective(vx_context context, vx_image src, vx_image dst)
{
    vx_float32 a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f, e = 0.0f, f = 0.0f, g = 1.0f, h = 0.0f, i = 0.0f;
    //! [warp perspective]
    // x0 = a x + b y + c;
    // y0 = d x + e y + f;
    // z0 = g x + h y + i;
    vx_float32 mat[3][3] = {
        {a, d, g}, // 'x' coefficients
        {b, e, h}, // 'y' coefficients
        {c, f, i}, // 'offsets'
    };
    vx_matrix matrix = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3);
    vxCopyMatrix(matrix, mat, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    //! [warp perspective]
    return vxuWarpPerspective(context, src, matrix, VX_INTERPOLATION_NEAREST_NEIGHBOR, dst);
}
