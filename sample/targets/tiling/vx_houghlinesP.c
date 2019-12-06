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

/*!
* \file
* \brief The Hough Lines Probabilistic Kernels
* \author Baichuan Su <baichuan@multicorewareinc.com>
*/

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>
#include "vx_interface.h"
#include "tiling.h"

static vx_status VX_CALLBACK vxHoughLinesPInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U1 || format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_array arr = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &arr, sizeof(arr));
            if (arr)
            {
                vx_enum item_type = 0;
                vxQueryArray(arr, VX_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
                if (item_type == VX_TYPE_HOUGH_LINES_PARAMS)
                {
                    status = VX_SUCCESS;
                }
                vxReleaseArray(&arr);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxHoughLinesPOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)node;

    if (index == 2)
    {
        ptr->type = VX_TYPE_ARRAY;
        ptr->dim.array.item_type = VX_TYPE_LINE_2D;
        ptr->dim.array.capacity = 0; /* no defined capacity requirement */
        status = VX_SUCCESS;
    }
    else if (index == 3)
    {
        ptr->dim.scalar.type = VX_TYPE_SIZE;
        status = VX_SUCCESS;
    }
    return status;
}

vx_tiling_kernel_t houghlinesp_kernel = 
{
    "org.khronos.openvx.tiling_hough_lines_probabilistic",
    VX_KERNEL_HOUGH_LINES_P,
    NULL,
    HoughLinesP_image_tiling_flexible,
    HoughLinesP_image_tiling_fast,
    4,
    { { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
      { VX_INPUT,  VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
      { VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
      { VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED }, },
    NULL,
    vxHoughLinesPInputValidator,
    vxHoughLinesPOutputValidator,
    NULL,
    NULL,
    { 32, 32 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};
