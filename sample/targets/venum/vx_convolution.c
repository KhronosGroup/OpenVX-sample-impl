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
 * \brief The Custom Convolution Kernel
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>

#if (VENUM_MAX_CONVOLUTION_DIM != VX_INT_MAX_CONVOLUTION_DIM)
#if defined(__GNUC__)
#error "Venum does not support VX required Convolution Size"
#endif
#endif

static vx_status VX_CALLBACK vxConvolveKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 3)
    {
        vx_border_t bordermode;
        vx_image       src  = (vx_image)parameters[0];
        vx_convolution conv = (vx_convolution)parameters[1];
        vx_image       dst  = (vx_image)parameters[2];
        status = vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));
        if (status == VX_SUCCESS)
        {
            status = vxConvolve(src, conv, dst, &bordermode);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxConvolveInputValidator(vx_node node, vx_uint32 index)
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

#if defined(OPENVX_USE_S16)
            if( (format == VX_DF_IMAGE_U8) || (format == VX_DF_IMAGE_S16) )
#else
            if (format == VX_DF_IMAGE_U8)
#endif
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    if (index == 1)
    {
        vx_image input = 0;
        vx_convolution conv = 0;

        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        vx_parameter param1 = vxGetParameterByIndex(node, index);

        vxQueryParameter(param0, VX_PARAMETER_REF, &input, sizeof(input));
        vxQueryParameter(param1, VX_PARAMETER_REF, &conv, sizeof(conv));
        if (input && conv)
        {
            vx_uint32 width = 0;
            vx_uint32 height = 0;
            vx_size dims[2] = { 0, 0 };

            vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));

            vxQueryConvolution(conv, VX_CONVOLUTION_COLUMNS, &dims[0], sizeof(dims[0]));
            vxQueryConvolution(conv, VX_CONVOLUTION_ROWS, &dims[1], sizeof(dims[1]));

            if ((dims[0] <= VX_INT_MAX_CONVOLUTION_DIM) &&
                (dims[1] <= VX_INT_MAX_CONVOLUTION_DIM) &&
                (width >= dims[0]) &&
                (height >= dims[1]))
            {
                status = VX_SUCCESS;
            }

            vxReleaseImage(&input);
            vxReleaseConvolution(&conv);
        }

        vxReleaseParameter(&param0);
        vxReleaseParameter(&param1);
    }

    return status;
}

static vx_status VX_CALLBACK vxConvolveOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter params[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, index),
        };
        if ((vxGetStatus((vx_reference)params[0]) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)params[1]) == VX_SUCCESS))
        {
            vx_image input = 0;
            vx_image output = 0;
            vxQueryParameter(params[0], VX_PARAMETER_REF, &input, sizeof(input));
            vxQueryParameter(params[1], VX_PARAMETER_REF, &output, sizeof(output));
            if (input && output)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = 0;
                vx_df_image output_format = 0;
                vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));

                vxQueryImage(output, VX_IMAGE_FORMAT, &output_format, sizeof(output_format));

                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = output_format == VX_DF_IMAGE_U8 ? VX_DF_IMAGE_U8 : VX_DF_IMAGE_S16;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;

                vxReleaseImage(&input);
                vxReleaseImage(&output);
            }
            vxReleaseParameter(&params[0]);
            vxReleaseParameter(&params[1]);
        }
    }
    return status;
}

static vx_param_description_t convolution_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_CONVOLUTION, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t convolution_kernel = {
    VX_KERNEL_CUSTOM_CONVOLUTION,
    "org.khronos.openvx.custom_convolution",
    vxConvolveKernel,
    convolution_kernel_params, dimof(convolution_kernel_params),
    NULL,
    vxConvolveInputValidator,
    vxConvolveOutputValidator,
    NULL,
    NULL,
};

