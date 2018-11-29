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
#include <vx_debug.h>

#include <VX/vx_types.h>
#include <VX/vx_lib_extras.h>

// nodeless version of the Phase kernel
vx_status vxPhase(vx_image grad_x, vx_image grad_y, vx_image output)
{
    vx_uint32 x;
    vx_uint32 y;
    vx_df_image format = 0;
    vx_uint8* dst_base = NULL;
    void* src_base_x   = NULL;
    void* src_base_y   = NULL;
    vx_imagepatch_addressing_t src_addr_x = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t src_addr_y = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr   = VX_IMAGEPATCH_ADDR_INIT;
    vx_rectangle_t rect;
    vx_status status = VX_FAILURE;

    if (grad_x == 0 && grad_y == 0)
        return VX_ERROR_INVALID_PARAMETERS;

    status  = VX_SUCCESS;

    status |= vxGetValidRegionImage(grad_x, &rect);

    status |= vxAccessImagePatch(grad_x, &rect, 0, &src_addr_x, &src_base_x, VX_READ_ONLY);
    status |= vxAccessImagePatch(grad_y, &rect, 0, &src_addr_y, &src_base_y, VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void**)&dst_base, VX_WRITE_ONLY);

    status |= vxQueryImage(grad_x, VX_IMAGE_FORMAT, &format, sizeof(format));

    for (y = 0; y < dst_addr.dim_y; y++)
    {
        for (x = 0; x < dst_addr.dim_x; x++)
        {
            void*     in_x = vxFormatImagePatchAddress2d(src_base_x, x, y, &src_addr_x);
            void*     in_y = vxFormatImagePatchAddress2d(src_base_y, x, y, &src_addr_y);
            vx_uint8* dst  = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

            /* -M_PI to M_PI */
            double val_x;
            double val_y;

            if (format == VX_DF_IMAGE_F32)
            {
                val_x = (double)(((vx_float32*)in_x)[0]);
                val_y = (double)(((vx_float32*)in_y)[0]);
            }
            else // VX_DF_IMAGE_S16
            {
                val_x = (double)(((vx_int16*)in_x)[0]);
                val_y = (double)(((vx_int16*)in_y)[0]);
            }

            double arct = atan2(val_y,val_x);
            /* 0 - TAU */
            double norm = arct;
            if (arct < 0.0f)
            {
                norm = VX_TAU + arct;
            }

            /* 0.0 - 1.0 */
            norm = norm / VX_TAU;

            /* 0 - 255 */
            *dst = (vx_uint8)((vx_uint32)(norm * 256u + 0.5) & 0xFFu);
            if (val_y != 0 || val_x != 0)
            {
                VX_PRINT(VX_ZONE_INFO, "atan2(%d,%d) = %lf [norm=%lf] dst=%02x\n", val_y, val_x, arct, norm, *dst);
            }
        }
    }

    status |= vxCommitImagePatch(grad_x, NULL, 0, &src_addr_x, src_base_x);
    status |= vxCommitImagePatch(grad_y, NULL, 0, &src_addr_y, src_base_y);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);

    return status;
}

