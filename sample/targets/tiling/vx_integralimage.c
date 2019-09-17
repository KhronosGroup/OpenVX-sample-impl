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

static vx_status VX_CALLBACK vxIntegralInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxIntegralOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0); /* we reference the input image */
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if (input)
            {
                vx_uint32 width = 0, height = 0;
                vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U32;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

vx_tiling_kernel_t integral_image_kernel =
{
    "org.khronos.openvx.tiling_integral_image",
    VX_KERNEL_INTEGRAL_IMAGE_TILING,
    IntegralImage_image_tiling_flexible,
    IntegralImage_image_tiling_fast,
    2,
    { { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED } },
    NULL,
    vxIntegralInputValidator,
    vxIntegralOutputValidator,
    { 16, 16 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};


