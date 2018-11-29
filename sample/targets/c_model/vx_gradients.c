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
 * \brief The Gradient Kernels (Base)
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include "vx_internal.h"
#include "c_model.h"

static
vx_param_description_t sobel3x3_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL },
};

static
vx_status VX_CALLBACK ownSobel3x3Kernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (NULL != node && NULL != parameters && num == dimof(sobel3x3_kernel_params))
    {
        vx_border_t bordermode;
        vx_image input  = (vx_image)parameters[0];
        vx_image grad_x = (vx_image)parameters[1];
        vx_image grad_y = (vx_image)parameters[2];
        status = vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));
        if (status == VX_SUCCESS)
        {
            status = vxSobel3x3(input, grad_x, grad_y, &bordermode);
        }
    }
    return status;
} /* ownSobel3x3Kernel() */

static
vx_status VX_CALLBACK own_sobel3x3_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(sobel3x3_kernel_params) && NULL != metas)
    {
        vx_parameter param1 = vxGetParameterByIndex(node, 0);
        vx_parameter param2 = vxGetParameterByIndex(node, 1);
        vx_parameter param3 = vxGetParameterByIndex(node, 2);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            ( (VX_SUCCESS == vxGetStatus((vx_reference)param2)) || (VX_SUCCESS == vxGetStatus((vx_reference)param3)) ))
        {
            vx_uint32   src_width  = 0;
            vx_uint32   src_height = 0;
            vx_df_image src_format = 0;
            vx_image    input = 0;

            status = vxQueryParameter(param1, VX_PARAMETER_REF, &input, sizeof(input));

            status |= vxQueryImage(input, VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
            status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
            status |= vxQueryImage(input, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));

            /* validate input image */
            if (VX_SUCCESS == status)
            {
                if (src_width >= 3 && src_height >= 3 && src_format == VX_DF_IMAGE_U8)
                    status = VX_SUCCESS;
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            /* validate output images */
            if (VX_SUCCESS == status)
            {
                vx_enum dst_format = VX_DF_IMAGE_S16;

                if (NULL == metas[1] && NULL == metas[2])
                    status = VX_ERROR_INVALID_PARAMETERS;

                if (VX_SUCCESS == status && NULL != metas[1])
                {
                    /* if optional parameter non NULL */
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                }

                if (VX_SUCCESS == status && NULL != metas[2])
                {
                    /* if optional parameter non NULL */
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                }
            }

            if (NULL != input)
                vxReleaseImage(&input);

            if (NULL != param1)
                vxReleaseParameter(&param1);

            if (NULL != param2)
                vxReleaseParameter(&param2);

            if (NULL != param3)
                vxReleaseParameter(&param3);
        }
    } /* if ptrs non NULL */

    return status;
} /* own_sobel3x3_validator() */

vx_kernel_description_t sobel3x3_kernel =
{
    VX_KERNEL_SOBEL_3x3,
    "org.khronos.openvx.sobel_3x3",
    ownSobel3x3Kernel,
    sobel3x3_kernel_params, dimof(sobel3x3_kernel_params),
    own_sobel3x3_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
