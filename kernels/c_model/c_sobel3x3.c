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

#include <c_model.h>

static vx_int16 sobel_x[3][3] = {
    {-1, 0, +1},
    {-2, 0, +2},
    {-1, 0, +1},
};

static vx_int16 sobel_y[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    {+1, +2, +1},
};

// nodeless version of the Sobel3x3 kernel
vx_status vxSobel3x3(vx_image input, vx_image grad_x, vx_image grad_y, vx_border_t *bordermode)
{
    if (grad_x) {
        vx_status status = vxConvolution3x3(input, grad_x, sobel_x, bordermode);
        if (status != VX_SUCCESS) return status;
    }

    if (grad_y) {
        vx_status status = vxConvolution3x3(input, grad_y, sobel_y, bordermode);
        if (status != VX_SUCCESS) return status;
    }

    return VX_SUCCESS;
}
