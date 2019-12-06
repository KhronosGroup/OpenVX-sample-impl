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
* \brief The Non-Maxima Suppression Kernels.
* \author Baichuan Su <baichuan@multicorewareinc.com>
*/

#include <vx_interface.h>
#include <tiling.h>
#include <vx_internal.h>
#include <VX/vx_helper.h>

static vx_status VX_CALLBACK vxNonMaxSuppressionInputValidator(vx_node node, vx_uint32 index)
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
            {
                status = VX_SUCCESS;
            }
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
            vx_df_image format;

            vxQueryImage(images[0], VX_IMAGE_WIDTH, &width[0], sizeof(width[0]));
            vxQueryImage(images[1], VX_IMAGE_WIDTH, &width[1], sizeof(width[1]));
            vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height[0], sizeof(height[0]));
            vxQueryImage(images[1], VX_IMAGE_HEIGHT, &height[1], sizeof(height[1]));
            vxQueryImage(images[1], VX_IMAGE_FORMAT, &format, sizeof(format));
            if ( (width[0] == width[1]) && (height[0] == height[1]) &&
                 (format == VX_DF_IMAGE_U1 || format == VX_DF_IMAGE_U8) )
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&images[0]);
            vxReleaseImage(&images[1]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    else if (index == 2)
    {
        vx_scalar win_size = 0;
        vx_image input = 0;
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 2)
        };

        vxQueryParameter(param[0], VX_PARAMETER_REF, &input, sizeof(input));
        vxQueryParameter(param[1], VX_PARAMETER_REF, &win_size, sizeof(win_size));
        if (input && win_size)
        {
            vx_enum type = 0;
            vx_uint32 width, height;
            vx_int32 wsize = 0;
            vxCopyScalar(win_size, &wsize, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            vxQueryScalar(win_size, VX_SCALAR_TYPE, &type, sizeof(type));
            vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
            if (type == VX_TYPE_INT32)
            {
                if ( (wsize <= width) && (wsize <= height) && (wsize % 2 == 1))
                {
                    status = VX_SUCCESS;
                }
            }
            else
            {
                status = VX_ERROR_INVALID_TYPE;
            }

            vxReleaseScalar(&win_size);
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    return status;
}

static vx_status VX_CALLBACK vxNonMaxSuppressionOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_image img = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &img, sizeof(img));
            if (img)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = 0;

                vxQueryImage(img, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(img, VX_IMAGE_HEIGHT, &height, sizeof(height));
                vxQueryImage(img, VX_IMAGE_FORMAT, &format, sizeof(format));
                
                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = format;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;

                status = VX_SUCCESS;
                vxReleaseImage(&img);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

vx_tiling_kernel_t nonmaxsuppression_kernel = 
{
    "org.khronos.openvx.tiling_nonmaxsuppression",
    VX_KERNEL_NON_MAX_SUPPRESSION,
    NULL,
    NonMaxSuppression_image_tiling_flexible,
    NonMaxSuppression_image_tiling_fast,
    4,
    { { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL },
    { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },},
	NULL,
    vxNonMaxSuppressionInputValidator,
    vxNonMaxSuppressionOutputValidator,
    NULL,
    NULL,
    { 16, 16 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};
