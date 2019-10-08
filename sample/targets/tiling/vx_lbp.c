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

#include "vx_interface.h"

#include "vx_internal.h"

#include "tiling.h"

static vx_status VX_CALLBACK vxLBPInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U8)
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
                    vx_enum format = 0;
                    vxCopyScalar(scalar, &format, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((format == VX_LBP) ||
                        (format == VX_MLBP) ||
                        (format == VX_ULBP))
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
            } //end if(scalar)
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
    {
        vx_enum format = 0;
        vx_parameter param_format = vxGetParameterByIndex(node, 1);
        if (vxGetStatus((vx_reference)param_format) == VX_SUCCESS)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param_format, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vxCopyScalar(scalar, &format, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param_format);
        }

        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar value = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &value, sizeof(value));
            if (value)
            {
                vx_enum stype = 0;
                vxQueryScalar(value, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_INT8)
                {
                    vx_int8 gs = 0;
                    vxCopyScalar(value, &gs, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ( (format == VX_LBP || format == VX_ULBP) &&
                         (gs == 3 || gs == 5))
                    {
                        status = VX_SUCCESS;
                    }
                    else if ( format == VX_MLBP && gs == 5 )
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
                vxReleaseScalar(&value);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxLBPOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{   
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
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

vx_tiling_kernel_t lbp_kernel = 
{
    "org.khronos.openvx.tiling_lbp",
    VX_KERNEL_LBP_TILING,
    NULL,
    LBP_image_tiling_flexible,
    LBP_image_tiling_fast,
    4,
    { { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
      { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
      { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
      { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED } },
    NULL,
    vxLBPInputValidator,
    vxLBPOutputValidator,
    NULL,
    NULL,
    { 16, 16 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};



