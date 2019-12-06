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
 * \brief The Bitwise Kernels: And, Or, Xor, Not.
 * \author Hans-Peter Nilsson <hp@axis.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>


/*
 * The three bitwise kernels with binary parameters have the same parameter domain so
 * let's just have one set of validators.
 */

static vx_status VX_CALLBACK vxBinaryBitwiseInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U1 || format == VX_DF_IMAGE_U8)
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
            vx_df_image format[2];

            vxQueryImage(images[0], VX_IMAGE_WIDTH, &width[0], sizeof(width[0]));
            vxQueryImage(images[1], VX_IMAGE_WIDTH, &width[1], sizeof(width[1]));
            vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height[0], sizeof(height[0]));
            vxQueryImage(images[1], VX_IMAGE_HEIGHT, &height[1], sizeof(height[1]));
            vxQueryImage(images[0], VX_IMAGE_FORMAT, &format[0], sizeof(format[0]));
            vxQueryImage(images[1], VX_IMAGE_FORMAT, &format[1], sizeof(format[1]));
            if (width[0] == width[1] && height[0] == height[1] && format[0] == format[1])
                status = VX_SUCCESS;
            vxReleaseImage(&images[1]);
            vxReleaseImage(&images[0]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    return status;
}

static vx_status VX_CALLBACK vxBinaryBitwiseOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        if (param0)
        {
            vx_image image0 = 0;
            vxQueryParameter(param0, VX_PARAMETER_REF, &image0, sizeof(image0));
            /*
             * When passing on the geometry to the output image, we only look at image 0, as
             * both input images are verified to match, at input validation.
             */
            if (image0)
            {
                vx_df_image format = 0;
                vx_uint32 width = 0, height = 0;
                vxQueryImage(image0, VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryImage(image0, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(image0, VX_IMAGE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = format;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&image0);
            }
            vxReleaseParameter(&param0);
        }
    }
    return status;
}

static vx_param_description_t binary_bitwise_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

static vx_status VX_CALLBACK vxAndKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == 3)
    {
        vx_image in1 = (vx_image)parameters[0];
        vx_image in2 = (vx_image)parameters[1];
        vx_image output = (vx_image)parameters[2];
        return vxAnd(in1, in2, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t and_kernel = {
    VX_KERNEL_AND,
    "org.khronos.openvx.and",
    vxAndKernel,
    binary_bitwise_kernel_params, dimof(binary_bitwise_kernel_params),
    NULL,
    vxBinaryBitwiseInputValidator,
    vxBinaryBitwiseOutputValidator,
    NULL,
    NULL,
};

static vx_status VX_CALLBACK vxOrKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == 3)
    {
        vx_image in1 = (vx_image)parameters[0];
        vx_image in2 = (vx_image)parameters[1];
        vx_image output = (vx_image)parameters[2];
        return vxOr(in1, in2, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t or_kernel = {
    VX_KERNEL_OR,
    "org.khronos.openvx.or",
    vxOrKernel,
    binary_bitwise_kernel_params, dimof(binary_bitwise_kernel_params),
    NULL,
    vxBinaryBitwiseInputValidator,
    vxBinaryBitwiseOutputValidator,
    NULL,
    NULL,
};

static vx_status VX_CALLBACK vxXorKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == 3)
    {
        vx_image in1 = (vx_image)parameters[0];
        vx_image in2 = (vx_image)parameters[1];
        vx_image output = (vx_image)parameters[2];
        return vxXor(in1, in2, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t xor_kernel = {
    VX_KERNEL_XOR,
    "org.khronos.openvx.xor",
    vxXorKernel,
    binary_bitwise_kernel_params, dimof(binary_bitwise_kernel_params),
    NULL,
    vxBinaryBitwiseInputValidator,
    vxBinaryBitwiseOutputValidator,
    NULL,
    NULL,
};

/* The Not kernel is an unary operator, requiring separate validators. */

static vx_status VX_CALLBACK vxUnaryBitwiseInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U1 || format == VX_DF_IMAGE_U8)
                status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxUnaryBitwiseOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_image inimage = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &inimage, sizeof(inimage));
            if (inimage)
            {
                vx_df_image format = 0;
                vx_uint32 width = 0, height = 0;
                vxQueryImage(inimage, VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryImage(inimage, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(inimage, VX_IMAGE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = format;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&inimage);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_param_description_t unary_bitwise_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};


static vx_status VX_CALLBACK vxNotKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == 2)
    {
        vx_image input = (vx_image)parameters[0];
        vx_image output = (vx_image)parameters[1];
        return vxNot(input, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

vx_kernel_description_t not_kernel = {
    VX_KERNEL_NOT,
    "org.khronos.openvx.not",
    vxNotKernel,
    unary_bitwise_kernel_params, dimof(unary_bitwise_kernel_params),
    NULL,
    vxUnaryBitwiseInputValidator,
    vxUnaryBitwiseOutputValidator,
    NULL,
    NULL,
};
