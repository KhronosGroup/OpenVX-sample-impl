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

#include "vx_interface.h"

#include "vx_internal.h"

#include <tiling.h>

static vx_status VX_CALLBACK vxPhaseInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (index == 0 || index == 1)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_S16 || format == VX_DF_IMAGE_F32)
            {
                if (index == 0)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    vx_parameter param0 = vxGetParameterByIndex(node, index);
                    vx_image input0 = 0;

                    vxQueryParameter(param0, VX_PARAMETER_REF, &input0, sizeof(input0));
                    if (input0)
                    {
                        vx_uint32 width0 = 0, height0 = 0, width1 = 0, height1 = 0;
                        vxQueryImage(input0, VX_IMAGE_WIDTH, &width0, sizeof(width0));
                        vxQueryImage(input0, VX_IMAGE_HEIGHT, &height0, sizeof(height0));
                        vxQueryImage(input, VX_IMAGE_WIDTH, &width1, sizeof(width1));
                        vxQueryImage(input, VX_IMAGE_HEIGHT, &height1, sizeof(height1));

                        if (width0 == width1 && height0 == height1)
                            status = VX_SUCCESS;

                        vxReleaseImage(&input0);
                    }

                    vxReleaseParameter(&param0);
                }
            }

            vxReleaseImage(&input);
        }

        vxReleaseParameter(&param);
    }

    return status;
}

static vx_status VX_CALLBACK vxPhaseOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (index == 2)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, 0);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_uint32   width = 0;
            vx_uint32   height = 0;
            vx_df_image format = 0;

            vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

            ptr->type = VX_TYPE_IMAGE;
            ptr->dim.image.format = VX_DF_IMAGE_U8;
            ptr->dim.image.width  = width;
            ptr->dim.image.height = height;

            status = VX_SUCCESS;

            vxReleaseImage(&input);
        }

        vxReleaseParameter(&param);
    }

    return status;
}

vx_tiling_kernel_t phase_kernel =
{
    "org.khronos.openvx.tiling_phase",
    VX_KERNEL_PHASE_TILING,
    Phase_image_tiling_flexible,
    Phase_image_tiling_fast,
    3,
    { { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED } },
    NULL,
    vxPhaseInputValidator,
    vxPhaseOutputValidator,
    { 8, 8 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};
