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
 * \brief The Min and Maximum Locations Kernel.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <venum.h>

static vx_status VX_CALLBACK vxMinMaxLocKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    if (num == 7)
    {
        vx_image input = (vx_image)parameters[0];
        vx_scalar minVal = (vx_scalar)parameters[1];
        vx_scalar maxVal = (vx_scalar)parameters[2];
        vx_array minLoc = (vx_array)parameters[3];
        vx_array maxLoc = (vx_array)parameters[4];
        vx_scalar minCount = (vx_scalar)parameters[5];
        vx_scalar maxCount = (vx_scalar)parameters[6];
        return vxMinMaxLoc(input, minVal, maxVal, minLoc, maxLoc, minCount, maxCount);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxMinMaxLocInputValidator(vx_node node, vx_uint32 index)
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
            if ((format == VX_DF_IMAGE_U8)
                || (format == VX_DF_IMAGE_S16)
#if defined(OPENVX_USE_S16)
                || (format == VX_DF_IMAGE_U16)
                || (format == VX_DF_IMAGE_U32)
                || (format == VX_DF_IMAGE_S32)
#endif
                )
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxMinMaxLocOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if ((index == 1) || (index == 2))
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if (input)
            {
                vx_df_image format;
                vx_enum type = VX_TYPE_INVALID;
                vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                switch (format)
                {
                    case VX_DF_IMAGE_U8:
                        type = VX_TYPE_UINT8;
                        break;
                    case VX_DF_IMAGE_U16:
                        type = VX_TYPE_UINT16;
                        break;
                    case VX_DF_IMAGE_U32:
                        type = VX_TYPE_UINT32;
                        break;
                    case VX_DF_IMAGE_S16:
                        type = VX_TYPE_INT16;
                        break;
                    case VX_DF_IMAGE_S32:
                        type = VX_TYPE_INT32;
                        break;
                    default:
                        type = VX_TYPE_INVALID;
                        break;
                }
                if (type != VX_TYPE_INVALID)
                {
                    status = VX_SUCCESS;
                    ptr->type = VX_TYPE_SCALAR;
                    ptr->dim.scalar.type = type;
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    if ((index == 3) || (index == 4))
    {
        /* nothing to check here */
        ptr->dim.array.item_type = VX_TYPE_COORDINATES2D;
        ptr->dim.array.capacity = 1;
        status = VX_SUCCESS;
    }
    if ((index == 5) || (index == 6))
    {
        ptr->dim.scalar.type = VX_TYPE_UINT32;
        status = VX_SUCCESS;
    }
    return status;
}

static vx_param_description_t minmaxloc_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
};

vx_kernel_description_t minmaxloc_kernel = {
    VX_KERNEL_MINMAXLOC,
    "org.khronos.openvx.minmaxloc",
    vxMinMaxLocKernel,
    minmaxloc_kernel_params, dimof(minmaxloc_kernel_params),
    NULL,
    vxMinMaxLocInputValidator,
    vxMinMaxLocOutputValidator,
    NULL,
    NULL,
};


