/*

 * Copyright (c) 2013-2017 The Khronos Group Inc.
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
 * \brief The Add and Subtract Kernels.
 * \author Hans-Peter Nilsson <hp@axis.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>


static vx_status VX_CALLBACK vxAddSubtractInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U8 || format == VX_DF_IMAGE_S16)
                status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_image images[2];
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        vxQueryParameter(param[0], VX_PARAMETER_REF, &images[0], sizeof(images[0]));
        vxQueryParameter(param[1], VX_PARAMETER_REF, &images[1], sizeof(images[1]));
        if (images[0] && images[1])
        {
            vx_uint32 width[2], height[2];
            vx_df_image format1;

            vxQueryImage(images[0], VX_IMAGE_WIDTH, &width[0], sizeof(width[0]));
            vxQueryImage(images[1], VX_IMAGE_WIDTH, &width[1], sizeof(width[1]));
            vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height[0], sizeof(height[0]));
            vxQueryImage(images[1], VX_IMAGE_HEIGHT, &height[1], sizeof(height[1]));
            vxQueryImage(images[1], VX_IMAGE_FORMAT, &format1, sizeof(format1));
            if (width[0] == width[1] && height[0] == height[1] &&
                (format1 == VX_DF_IMAGE_U8 || format1 == VX_DF_IMAGE_S16))
                status = VX_SUCCESS;
            vxReleaseImage(&images[0]);
            vxReleaseImage(&images[1]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    else if (index == 2)        /* overflow_policy: truncate or saturate. */
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
                    vx_enum overflow_policy = 0;
                    vxCopyScalar(scalar, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((overflow_policy == VX_CONVERT_POLICY_WRAP) ||
                        (overflow_policy == VX_CONVERT_POLICY_SATURATE))
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
    return status;
}

static vx_status VX_CALLBACK vxAddSubtractOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        /*
         * We need to look at both input images, but only for the format:
         * if either is S16 or the output type is not U8, then it's S16.
         * The geometry of the output image is copied from the first parameter:
         * the input images are known to match from input parameters validation.
         */
        vx_parameter param[] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
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
                vx_df_image informat[2] = {VX_DF_IMAGE_VIRT, VX_DF_IMAGE_VIRT};
                vx_df_image outformat = VX_DF_IMAGE_VIRT;

                /*
                 * When passing on the geometry to the output image, we only look at
                 * image 0, as both input images are verified to match, at input
                 * validation.
                 */
                vxQueryImage(images[0], VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height, sizeof(height));
                vxQueryImage(images[0], VX_IMAGE_FORMAT, &informat[0], sizeof(informat[0]));
                vxQueryImage(images[1], VX_IMAGE_FORMAT, &informat[1], sizeof(informat[1]));
                vxQueryImage(images[2], VX_IMAGE_FORMAT, &outformat, sizeof(outformat));

                if (informat[0] == VX_DF_IMAGE_U8 && informat[1] == VX_DF_IMAGE_U8 && outformat == VX_DF_IMAGE_U8)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    outformat = VX_DF_IMAGE_S16;
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


static vx_param_description_t add_subtract_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

/* There's already a "vxAddKernel"; we have to use a slightly different name. */
static vx_status VX_CALLBACK vxAdditionKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == dimof(add_subtract_kernel_params))
    {
        vx_image  in0          =  (vx_image)parameters[0];
        vx_image  in1          =  (vx_image)parameters[1];
        vx_scalar policy_param = (vx_scalar)parameters[2];
        vx_image  output       =  (vx_image)parameters[3];

        return vxAddition(in0, in1, policy_param, output);
    }

    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t add_kernel = {
    VX_KERNEL_ADD,
    "org.khronos.openvx.add",
    vxAdditionKernel,
    add_subtract_kernel_params, dimof(add_subtract_kernel_params),
    NULL,
    vxAddSubtractInputValidator,
    vxAddSubtractOutputValidator,
    NULL,
    NULL,
};

static vx_status VX_CALLBACK vxSubtractionKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == dimof(add_subtract_kernel_params))
    {
        vx_image  in0           =  (vx_image)parameters[0];
        vx_image  in1           =  (vx_image)parameters[1];
        vx_scalar policy_param  = (vx_scalar)parameters[2];
        vx_image  output        =  (vx_image)parameters[3];

        return vxSubtraction(in0, in1, policy_param, output);
    }

    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t subtract_kernel = {
    VX_KERNEL_SUBTRACT,
    "org.khronos.openvx.subtract",
    vxSubtractionKernel,
    add_subtract_kernel_params, dimof(add_subtract_kernel_params),
    NULL,
    vxAddSubtractInputValidator,
    vxAddSubtractOutputValidator,
    NULL,
    NULL,
};

