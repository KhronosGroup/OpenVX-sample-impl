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

#define PATCH_DIM 16

vx_status example_imagepatch(vx_context context)
{
    //! [imagepatch]
    vx_status status = VX_SUCCESS;
    void *base_ptr = NULL;
    vx_uint32 width = 640, height = 480, plane = 0;
    vx_image image = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t addr;
    vx_map_id map_id;

    rect.start_x = rect.start_y = 0;
    rect.end_x = rect.end_y = PATCH_DIM;

    status = vxMapImagePatch(image, &rect, plane, &map_id,
                                &addr, &base_ptr,
                                VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
    if (status == VX_SUCCESS)
    {
        vx_uint32 x,y,i,j;
        vx_uint8 pixel = 0;

        /* a couple addressing options */

        /* use linear addressing function/macro */
        for (i = 0; i < addr.dim_x*addr.dim_y; i++) {
            vx_uint8 *ptr2 = vxFormatImagePatchAddress1d(base_ptr,
                                                         i, &addr);
            *ptr2 = pixel;
        }

        /* 2d addressing option */
        for (y = 0; y < addr.dim_y; y+=addr.step_y) {
            for (x = 0; x < addr.dim_x; x+=addr.step_x) {
                vx_uint8 *ptr2 = vxFormatImagePatchAddress2d(base_ptr,
                                                             x, y, &addr);
                *ptr2 = pixel;
            }
        }

        /* direct addressing by client
         * for subsampled planes, scale will change
         */
        for (y = 0; y < addr.dim_y; y+=addr.step_y) {
            for (x = 0; x < addr.dim_x; x+=addr.step_x) {
                vx_uint8 *tmp = (vx_uint8 *)base_ptr;
                i = ((addr.stride_y*y*addr.scale_y) /
                      VX_SCALE_UNITY) +
                    ((addr.stride_x*x*addr.scale_x) /
                      VX_SCALE_UNITY);
                tmp[i] = pixel;
            }
        }

        /* more efficient direct addressing by client.
         * for subsampled planes, scale will change.
         */
        for (y = 0; y < addr.dim_y; y+=addr.step_y) {
            j = (addr.stride_y*y*addr.scale_y)/VX_SCALE_UNITY;
            for (x = 0; x < addr.dim_x; x+=addr.step_x) {
                vx_uint8 *tmp = (vx_uint8 *)base_ptr;
                i = j + (addr.stride_x*x*addr.scale_x) /
                    VX_SCALE_UNITY;
                tmp[i] = pixel;
            }
        }

        /* this commits the data back to the image.
         */
        status = vxUnmapImagePatch(image, map_id);
    }
    vxReleaseImage(&image);
    //! [imagepatch]
    return status;
}

