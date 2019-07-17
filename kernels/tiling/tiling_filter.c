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


#include <tiling.h>

void box3x3_image_tiling(void * parameters[], void * tile_memory, vx_size tile_memory_size)
{
    vx_uint32 x, y;
    vx_tile_t *in = (vx_tile_t *)parameters[0];
    vx_tile_t *out = (vx_tile_t *)parameters[1];

    for (y = 1; y < vxTileHeight(out, 0); y += vxTileBlockHeight(out))
    {
        for (x = 1; x < vxTileWidth(out, 0); x += vxTileBlockWidth(out))
        {
            vx_int32 j, i;
            vx_uint32 sum = 0;
            vx_uint32 count = 0;

            for (j = vxNeighborhoodTop(in); j <= vxNeighborhoodBottom(in); j++)
            {
                for (i = vxNeighborhoodLeft(in); i <= vxNeighborhoodRight(in); i++)
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