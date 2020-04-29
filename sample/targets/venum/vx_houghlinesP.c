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
#include <venum.h>

static vx_status VX_CALLBACK vxHoughLinesPKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)node;

    if (num == 4)
    {
        vx_image img = (vx_image)parameters[0];
        vx_array param_hough_lines = (vx_array)parameters[1];
        vx_array lines_array = (vx_array)parameters[2];
        vx_scalar num_lines = (vx_scalar)parameters[3];
        status = vxHoughLinesP(img, param_hough_lines, lines_array, num_lines);
    }
    return status;
}

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

static vx_param_description_t houghlinesp_kernel_params[] = {
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
};

vx_kernel_description_t houghlinesp_kernel = {
    VX_KERNEL_HOUGH_LINES_P,
    "org.khronos.openvx.hough_lines_p",
    vxHoughLinesPKernel,
    houghlinesp_kernel_params, dimof(houghlinesp_kernel_params),
    NULL,
    vxHoughLinesPInputValidator,
    vxHoughLinesPOutputValidator,
    NULL,
    NULL,
};
