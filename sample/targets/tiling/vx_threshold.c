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

#include "tiling.h"

static vx_status VX_CALLBACK vxThresholdInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if (input)
            {
                vx_df_image format = 0;
                vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                if ((format == VX_DF_IMAGE_U8 || format == VX_DF_IMAGE_S16))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_FORMAT;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_threshold threshold = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &threshold, sizeof(threshold));
            if (threshold)
            {
                vx_enum type = 0;
                vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));
                if ((type == VX_THRESHOLD_TYPE_BINARY) ||
                     (type == VX_THRESHOLD_TYPE_RANGE))
                {
                    vx_enum data_type = 0;
                    vxQueryThreshold(threshold, VX_THRESHOLD_DATA_TYPE, &data_type, sizeof(data_type));
                    if (data_type == VX_TYPE_UINT8)
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_TYPE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseThreshold(&threshold);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxThresholdOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        if (vxGetStatus((vx_reference)src_param) == VX_SUCCESS)
        {
            vx_image src = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            if (src)
            {
                vx_uint32 width = 0, height = 0;

                vxQueryImage(src, VX_IMAGE_WIDTH, &width, sizeof(height));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &height, sizeof(height));

                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U8;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&src);
            }
            vxReleaseParameter(&src_param);
        }
    }
    return status;
}

vx_tiling_kernel_t threshold_kernel = 
{
    "org.khronos.openvx.tiling_threshold",
    VX_KERNEL_THRESHOLD_TILING,
    Threshold_image_tiling_flexible,
    Threshold_image_tiling_fast,
    3, 
    { { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_THRESHOLD,   VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED } },
    NULL,
    vxThresholdInputValidator,
    vxThresholdOutputValidator,
    { 16, 16 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};
