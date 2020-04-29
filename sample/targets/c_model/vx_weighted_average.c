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
* \brief The weight average Locations Kernel.
* \author ZHaojun Fan <zhaojun@multicorewareinc.com>
*/

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <c_model.h>

static vx_param_description_t weightedaverage_kernel_params[] = {
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static vx_status VX_CALLBACK vxWeightedAverageKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == dimof(weightedaverage_kernel_params))
    {
        vx_image in0 = (vx_image)parameters[0];
        vx_scalar alpha = (vx_scalar)parameters[1];
        vx_image in1 = (vx_image)parameters[2];
        vx_image out = (vx_image)parameters[3];
        return vxWeightedAverage(in0, alpha, in1, out);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxWeightedAverageInputValidator(vx_node node, vx_uint32 index)
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
            if ((format == VX_DF_IMAGE_U8))
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_scalar scalar = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum type = -1;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    vx_float32 scale = 0.0f;
                    if ((vxCopyScalar(scalar, &scale, VX_READ_ONLY, VX_MEMORY_TYPE_HOST) == VX_SUCCESS) &&
                        (scale >= 0) && (scale <= 1.0))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
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

static vx_status VX_CALLBACK vxWeightedAverageOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter param[] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 2),
            vxGetParameterByIndex(node, index),
        };
        if ((vxGetStatus((vx_reference)param[0]) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)param[1]) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)param[2]) == VX_SUCCESS))
        {
            vx_image images[3];
            vxQueryParameter(param[0], VX_PARAMETER_REF, &images[0], sizeof(images[0]));
            vxQueryParameter(param[1], VX_PARAMETER_REF, &images[1], sizeof(images[1]));
            vxQueryParameter(param[2], VX_PARAMETER_REF, &images[2], sizeof(images[2]));
            if (images[0] && images[1] && images[2])
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image informat[2] = { VX_DF_IMAGE_VIRT, VX_DF_IMAGE_VIRT };
                vx_df_image outformat = VX_DF_IMAGE_VIRT;
                vxQueryImage(images[0], VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height, sizeof(height));
                vxQueryImage(images[0], VX_IMAGE_FORMAT, &informat[0], sizeof(informat[0]));
                vxQueryImage(images[1], VX_IMAGE_FORMAT, &informat[1], sizeof(informat[1]));
                vxQueryImage(images[2], VX_IMAGE_FORMAT, &outformat, sizeof(outformat));
                if (informat[0] == VX_DF_IMAGE_U8 && informat[1] == VX_DF_IMAGE_U8 && outformat == VX_DF_IMAGE_U8)
                {
                    status = VX_SUCCESS;
                }
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = outformat;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                vxReleaseImage(&images[0]);
                vxReleaseImage(&images[1]);
                vxReleaseImage(&images[2]);
            }
            vxReleaseParameter(&param[0]);
            vxReleaseParameter(&param[1]);
            vxReleaseParameter(&param[2]);
        }
    }
    return status;
}

vx_kernel_description_t weightedaverage_kernel = {
    VX_KERNEL_WEIGHTED_AVERAGE,
    "org.khronos.openvx.weighted_average",
    vxWeightedAverageKernel,
    weightedaverage_kernel_params, dimof(weightedaverage_kernel_params),
    NULL,
    vxWeightedAverageInputValidator,
    vxWeightedAverageOutputValidator,
    NULL,
    NULL,
};