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

static vx_status VX_CALLBACK vxFast9InputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if ((status == VX_SUCCESS) && (input))
            {
                vx_df_image format = 0;
                status = vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                if ((status == VX_SUCCESS) && (format == VX_DF_IMAGE_U8))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar sens = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &sens, sizeof(sens));
            if ((status == VX_SUCCESS) && (sens))
            {
                vx_enum type = VX_TYPE_INVALID;
                vxQueryScalar(sens, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    vx_float32 k = 0.0f;
                    status = vxCopyScalar(sens, &k, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((status == VX_SUCCESS) && (k > 0) && (k < 256))
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
                vxReleaseScalar(&sens);
            }
            vxReleaseParameter(&param);
        }
    }
    if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar s_nonmax = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &s_nonmax, sizeof(s_nonmax));
            if ((status == VX_SUCCESS) && (s_nonmax))
            {
                vx_enum type = VX_TYPE_INVALID;
                vxQueryScalar(s_nonmax, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_BOOL)
                {
                    vx_bool nonmax = vx_false_e;
                    status = vxCopyScalar(s_nonmax, &nonmax, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((status == VX_SUCCESS) && ((nonmax == vx_false_e) ||
                                                   (nonmax == vx_true_e)))
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
                vxReleaseScalar(&s_nonmax);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxFast9OutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        ptr->type = VX_TYPE_ARRAY;
        ptr->dim.array.item_type = VX_TYPE_KEYPOINT;
        ptr->dim.array.capacity = 0; /* no defined capacity requirement */
        status = VX_SUCCESS;
    }
    else if (index == 4)
    {
        ptr->dim.scalar.type = VX_TYPE_SIZE;
        status = VX_SUCCESS;
    }
    return status;
}

vx_tiling_kernel_t fast9_kernel =
{
    "org.khronos.openvx.tiling_fast_corners",
    VX_KERNEL_FAST_CORNERS_TILING,
    NULL,
    Fast9Corners_image_tiling_flexible,
    Fast9Corners_image_tiling_fast,
    5,
    { { VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
      { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
      { VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
      { VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED },
      { VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL } },
    NULL,
    vxFast9InputValidator,
    vxFast9OutputValidator,
    NULL,
    NULL,
    { 16, 16 },
    { -1, 1, -1, 1 },
    { VX_BORDER_MODE_UNDEFINED, 0 },
};
