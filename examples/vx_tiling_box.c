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

#include <stdio.h>
#include <VX/vx_khr_tiling.h>

/*! \file
 * \example vx_tiling_box.c
 */

/*! \brief A 3x3 to 1x1 box filter
 * This kernel uses this tiling definition.
 * \code
 * vx_block_size_t outSize = {1,1};
 * vx_neighborhood_size_t inNbhd = {-1,1,-1,1};
 * \endcode
 * \ingroup group_tiling
 */
//! [box_tiling_function]
void box_image_tiling(void * VX_RESTRICT parameters[VX_RESTRICT],
                      void * VX_RESTRICT tile_memory,
                      vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_tile_t *out = (vx_tile_t *)parameters[1];

    for (y = 0; y < vxTileHeight(out, 0); y+=vxTileBlockHeight(out))
    {
        for (x = 0; x < vxTileWidth(out, 0); x+=vxTileBlockWidth(out))
        {
            vx_int32 j, i;
            vx_uint32 sum = 0;
            vx_uint32 count = 0;
            /* these loops can handle 3x3, 5x5, etc. since block size would be 1x1 */
            for (j = vxNeighborhoodTop(in); j < vxNeighborhoodBottom(in); j++)
            {
                for (i = vxNeighborhoodLeft(in); i < vxNeighborhoodRight(in); i++)
                {
                    sum += vxImagePixel(vx_uint8, in, 0, x, y, i, j);
                    count++;
                }
            }
            sum /= count;
            if (sum > 255)
                sum = 255;
            vxImagePixel(vx_uint8, out, 0, x, y, 0, 0) = (vx_uint8)sum;
        }
    }
}
//! [box_tiling_function]

