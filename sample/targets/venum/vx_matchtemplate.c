/*

 * Copyright (c) 2017-2017 The Khronos Group Inc.
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
 * \brief The Match Template Kernel
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include <vx_internal.h>
#include <venum.h>
#include <stdio.h>
#include <math.h>

/* match template kernel parameters*/
static vx_param_description_t match_template_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
};
/* match template kernel */
static vx_status VX_CALLBACK vxMatchTemplateKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == dimof( match_template_kernel_params))
    {
        vx_image  src = (vx_image)parameters[0];
        vx_image  template_image = (vx_image)parameters[1];
        vx_scalar matchingMethod = (vx_scalar)parameters[2];
        vx_image  output = (vx_image)parameters[3];

        return vxMatchTemplate(src, template_image, matchingMethod, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxMatchTemplateInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
                if(index == 1)
                {
                    vx_uint32 width = 0;
                    vx_uint32 height = 0;

                    vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
                    vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
                    if(width * height > 65535)
                    {
                        printf("The size of template image is larger than 65535.\n");
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vx_enum metric = 0;
                    vxCopyScalar(scalar, &metric, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((metric == VX_COMPARE_HAMMING) ||
                        (metric == VX_COMPARE_L1) ||
                        (metric == VX_COMPARE_L2) ||
                        (metric == VX_COMPARE_CCORR) ||
                        (metric == VX_COMPARE_L2_NORM) ||
                        (metric == VX_COMPARE_CCORR_NORM))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        printf("Matching Method given as %08x\n", metric);
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
    return status;
}

static vx_status VX_CALLBACK vxMatchTemplateOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_image output = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &output, sizeof(output));
        if (output)
        {
            vx_df_image format = 0;
            vxQueryImage(output, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_S16)
            {
                status = VX_SUCCESS;
            }
            vx_uint32 width = 0, height = 0;
            vxQueryImage(output, VX_IMAGE_WIDTH, &width, sizeof(width));
            vxQueryImage(output, VX_IMAGE_HEIGHT, &height, sizeof(height));
            ptr->type = VX_TYPE_IMAGE;
            ptr->dim.image.format = format;
            ptr->dim.image.width = width;
            ptr->dim.image.height = height;

            vxReleaseImage(&output);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

vx_kernel_description_t match_template_kernel =
{
    VX_KERNEL_MATCH_TEMPLATE,
    "org.khronos.openvx.match_template",
    vxMatchTemplateKernel,
    match_template_kernel_params, dimof(match_template_kernel_params),
    NULL,
    vxMatchTemplateInputValidator,
    vxMatchTemplateOutputValidator,
    NULL,
    NULL,
};
