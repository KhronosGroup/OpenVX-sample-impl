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
 * \brief The Thresholding Kernel.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>

static vx_status VX_CALLBACK vxThresholdKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == 3)
    {
        vx_image     src_image = (vx_image)    parameters[0];
        vx_threshold threshold = (vx_threshold)parameters[1];
        vx_image     dst_image = (vx_image)    parameters[2];
        return vxThreshold(src_image, threshold, dst_image);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

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
        vx_parameter thr_param = vxGetParameterByIndex(node, index);
        vx_parameter img_param = vxGetParameterByIndex(node, 2);
        if ((vxGetStatus((vx_reference)thr_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)img_param) == VX_SUCCESS))
        {
            vx_threshold threshold = 0;
            vx_image     output = 0;
            vxQueryParameter(thr_param, VX_PARAMETER_REF, &threshold, sizeof(threshold));
            vxQueryParameter(img_param, VX_PARAMETER_REF, &output,    sizeof(output));
            if (threshold && output)
            {
                vx_enum type = 0;
                vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));
                if ((type == VX_THRESHOLD_TYPE_BINARY) ||
                    (type == VX_THRESHOLD_TYPE_RANGE))
                {
                    vx_enum     data_type = 0;
                    vx_df_image format = 0;
                    vxQueryThreshold(threshold, VX_THRESHOLD_DATA_TYPE, &data_type, sizeof(data_type));
                    vxQueryImage(output,        VX_IMAGE_FORMAT,        &format,    sizeof(format));
                    if (data_type == VX_TYPE_UINT8 && format == VX_DF_IMAGE_U8)
                    {
                        status = VX_SUCCESS;
                    }
                    else if (data_type == VX_TYPE_BOOL && format == VX_DF_IMAGE_U1)
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
                vxReleaseImage(&output);
            }
            vxReleaseParameter(&thr_param);
            vxReleaseParameter(&img_param);
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
        vx_parameter dst_param = vxGetParameterByIndex(node, 2);
        if ((vxGetStatus((vx_reference)src_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS))
        {
            vx_image src = 0, dst = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            if (src && dst)
            {
                vx_df_image in_format = 0, out_format = 0;
                vx_uint32 width = 0, height = 0;

                vxQueryImage(src, VX_IMAGE_WIDTH,  &width,      sizeof(height));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &height,     sizeof(height));
                vxQueryImage(src, VX_IMAGE_FORMAT, &in_format,  sizeof(in_format));
                vxQueryImage(dst, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));

                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = out_format;
                ptr->dim.image.width  = width;
                ptr->dim.image.height = height;

                if ((out_format == VX_DF_IMAGE_U1 && in_format == VX_DF_IMAGE_U8) ||
                    (out_format == VX_DF_IMAGE_U8))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_FORMAT;
                }

                vxReleaseImage(&src);
                vxReleaseImage(&dst);
            }
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

static vx_param_description_t threshold_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_THRESHOLD,   VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT,VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t threshold_kernel = {
    VX_KERNEL_THRESHOLD,
    "org.khronos.openvx.threshold",
    vxThresholdKernel,
    threshold_kernel_params, dimof(threshold_kernel_params),
    NULL,
    vxThresholdInputValidator,
    vxThresholdOutputValidator,
    NULL,
    NULL,
};
