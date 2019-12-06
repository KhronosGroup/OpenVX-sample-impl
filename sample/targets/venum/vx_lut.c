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
 * \brief The TableLookup Kernel.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */
#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>

#include <math.h>

static vx_param_description_t lut_kernel_params[] = {
    { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT, VX_TYPE_LUT, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static vx_status VX_CALLBACK vxTableLookupKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == dimof(lut_kernel_params))
    {
        vx_image src_image =  (vx_image)parameters[0];
        vx_lut   lut       =    (vx_lut)parameters[1];
        vx_image dst_image =  (vx_image)parameters[2];

        return vxTableLookup(src_image, lut, dst_image);
    }

    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxTableLookupInputValidator(vx_node node, vx_uint32 index)
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
        vx_parameter param = vxGetParameterByIndex(node, index);
        vx_lut lut = 0;
        vxQueryParameter(param, VX_PARAMETER_REF, &lut, sizeof(lut));
        if (lut)
        {
            vx_enum type = 0;
            vxQueryLUT(lut, VX_LUT_TYPE, &type, sizeof(type));
            if (type == VX_TYPE_UINT8 || type == VX_TYPE_INT16)
            {
                status = VX_SUCCESS;
            }
            vxReleaseLUT(&lut);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxTableLookupOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        if (vxGetStatus((vx_reference)src_param) == VX_SUCCESS)
        {
            vx_image src = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            if (src)
            {
                vx_df_image format = 0;
                vx_uint32 width = 0, height = 0;

                vxQueryImage(src, VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryImage(src, VX_IMAGE_WIDTH, &width, sizeof(height));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &height, sizeof(height));
                /* output is equal type and size */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = format;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&src);
            }
            vxReleaseParameter(&src_param);
        }
    }
    return status;
}

vx_kernel_description_t lut_kernel = {
    VX_KERNEL_TABLE_LOOKUP,
    "org.khronos.openvx.table_lookup",
    vxTableLookupKernel,
    lut_kernel_params, dimof(lut_kernel_params),
    NULL,
    vxTableLookupInputValidator,
    vxTableLookupOutputValidator,
    NULL,
    NULL,
};


