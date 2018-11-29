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

vx_status example_conv(vx_context context)
{
    //! [assign]
    // A horizontal Scharr gradient operator with different scale.
    vx_int16 gx[3][3] = {
        {  3, 0, -3},
        { 10, 0,-10},
        {  3, 0, -3},
    };
    vx_uint32 scale = 8;
    vx_convolution scharr_x = vxCreateConvolution(context, 3, 3);
    vxCopyConvolutionCoefficients(scharr_x, (vx_int16*)gx, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    vxSetConvolutionAttribute(scharr_x, VX_CONVOLUTION_SCALE, &scale, sizeof(scale));
    //! [assign]
    vxReleaseConvolution(&scharr_x);
    return VX_SUCCESS;
}
