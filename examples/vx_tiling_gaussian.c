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

#include <VX/vx_khr_tiling.h>

/*! \file
 * \example vx_tiling_gaussian.c
 */

/*! \brief A 3x3 to 1x1 tiling Gaussian Blur.
 * This kernel uses this tiling definition.
 * \code
 * vx_block_size_t outSize = {1,1};
 * vx_neighborhood_size_t inNhbd = {-1,1,-1,1};
 * \endcode
 * \ingroup group_tiling
 */
//!  [gaussian_tiling_function]
void gaussian_image_tiling_fast(void * VX_RESTRICT parameters[VX_RESTRICT],
                                void * VX_RESTRICT tile_memory,
                                vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_tile_t *out = (vx_tile_t *)parameters[1];

    for (y = 0; y < vxTileHeight(out, 0); y += vxTileBlockHeight(out))
    {
        for (x = 0; x < vxTileWidth(out, 0); x += vxTileBlockWidth(out))
        {
            vx_uint32 sum = 0;
            /* this math covers a {-1,1},{-1,1} neighborhood and a block of 1x1.
             * an internal set of for loops could have been placed here instead.
             */
            sum += vxImagePixel(vx_uint8, in, 0, x, y, -1, -1);
            sum += vxImagePixel(vx_uint8, in, 0, x, y,  0, -1) << 1;
            sum += vxImagePixel(vx_uint8, in, 0, x, y, +1, -1);
            sum += vxImagePixel(vx_uint8, in, 0, x, y, -1,  0) << 1;
            sum += vxImagePixel(vx_uint8, in, 0, x, y,  0,  0) << 2;
            sum += vxImagePixel(vx_uint8, in, 0, x, y, +1,  0) << 1;
            sum += vxImagePixel(vx_uint8, in, 0, x, y, -1, +1);
            sum += vxImagePixel(vx_uint8, in, 0, x, y,  0, +1) << 1;
            sum += vxImagePixel(vx_uint8, in, 0, x, y, +1, +1);
            sum >>= 4;
            if (sum > 255)
                sum = 255;
            vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
        }
    }
}
//! [gaussian_tiling_function]


