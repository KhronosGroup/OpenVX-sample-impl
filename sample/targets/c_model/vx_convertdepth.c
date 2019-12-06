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
 * \brief The Convert Depth Kernel
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <c_model.h>


static vx_status VX_CALLBACK vxConvertDepthKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    (void)node;

    if (num == 4)
    {
        vx_image  input  = (vx_image) parameters[0];
        vx_image  output = (vx_image) parameters[1];
        vx_scalar spol   = (vx_scalar)parameters[2];
        vx_scalar sshf   = (vx_scalar)parameters[3];
        return vxConvertDepth(input, output, spol, sshf);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxConvertDepthInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if ((status == VX_SUCCESS) && input)
            {
                vx_df_image format = 0;
                status = vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                if ((status != VX_SUCCESS) ||
                    (format == VX_DF_IMAGE_U1)  ||
                    (format == VX_DF_IMAGE_U8)  ||
#if defined(OPENVX_USE_S16)
                    (format == VX_DF_IMAGE_U16) ||
                    (format == VX_DF_IMAGE_U32) ||
                    (format == VX_DF_IMAGE_S32) ||
                    (format == VX_DF_IMAGE_F32) ||
#endif
                    (format == VX_DF_IMAGE_S16))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
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
                    vx_enum overflow_policy = 0;
                    vxCopyScalar(scalar, &overflow_policy, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((overflow_policy == VX_CONVERT_POLICY_WRAP) ||
                        (overflow_policy == VX_CONVERT_POLICY_SATURATE))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        printf("Overflow given as %08x\n", overflow_policy);
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
    else if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar scalar = 0;
            status = vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (status == VX_SUCCESS)
            {
                vx_enum type = 0;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_INT32)
                {
                    vx_int32 shift = 0;
                    status = vxCopyScalar(scalar, &shift, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if (status == VX_SUCCESS)
                    {
                        /*! \internal Allowing \f$ 0 \le shift < 32 \f$ could
                         * produce weird results for smaller bit depths */
                        if (shift < 0 || shift >= 32)
                        {
                            status = VX_ERROR_INVALID_VALUE;
                        }
                        /* status should be VX_SUCCESS from call */
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

static vx_status VX_CALLBACK vxConvertDepthOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        if ((vxGetStatus((vx_reference)param[0]) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)param[1]) == VX_SUCCESS))
        {
            vx_image images[2] = {0,0};
            status  = VX_SUCCESS;
            status |= vxQueryParameter(param[0], VX_PARAMETER_REF, &images[0], sizeof(images[0]));
            status |= vxQueryParameter(param[1], VX_PARAMETER_REF, &images[1], sizeof(images[1]));
            if ((status == VX_SUCCESS) && (images[0]) && (images[1]))
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format[2] = {VX_DF_IMAGE_VIRT, VX_DF_IMAGE_VIRT};
                status |= vxQueryImage(images[0], VX_IMAGE_WIDTH, &width, sizeof(width));
                status |= vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height, sizeof(height));
                status |= vxQueryImage(images[0], VX_IMAGE_FORMAT, &format[0], sizeof(format[0]));
                status |= vxQueryImage(images[1], VX_IMAGE_FORMAT, &format[1], sizeof(format[1]));
                if (((format[0] == VX_DF_IMAGE_U1)  && (format[1] == VX_DF_IMAGE_U8))  ||
                    ((format[0] == VX_DF_IMAGE_U1)  && (format[1] == VX_DF_IMAGE_S16)) ||
                    ((format[0] == VX_DF_IMAGE_U8)  && (format[1] == VX_DF_IMAGE_U1))  ||
                    ((format[0] == VX_DF_IMAGE_U8)  && (format[1] == VX_DF_IMAGE_S16)) ||
#if defined(OPENVX_USE_S16)
                    ((format[0] == VX_DF_IMAGE_U1)  && (format[1] == VX_DF_IMAGE_U16)) ||
                    ((format[0] == VX_DF_IMAGE_U1)  && (format[1] == VX_DF_IMAGE_U32)) ||
                    ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U1))  ||
                    ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U1))  ||
                    ((format[0] == VX_DF_IMAGE_U8)  && (format[1] == VX_DF_IMAGE_U16)) ||
                    ((format[0] == VX_DF_IMAGE_U8)  && (format[1] == VX_DF_IMAGE_U32)) ||
                    ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U8))  ||
                    ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U32)) ||
                    ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_S32)) ||
                    ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U8))  ||
                    ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U16)) ||
                    ((format[0] == VX_DF_IMAGE_S32) && (format[1] == VX_DF_IMAGE_S16)) ||
                    ((format[0] == VX_DF_IMAGE_F32) && (format[1] == VX_DF_IMAGE_U8))  || /* non-specification */
#endif
                    ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_U1))  ||
                    ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_U8)))
                {
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = format[1];
                    ptr->dim.image.width = width;
                    ptr->dim.image.height = height;
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                vxReleaseImage(&images[0]);
                vxReleaseImage(&images[1]);
            }
            vxReleaseParameter(&param[0]);
            vxReleaseParameter(&param[1]);
        }
    }
    return status;
}

static vx_param_description_t convertdepth_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR,  VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR,  VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t convertdepth_kernel = {
    VX_KERNEL_CONVERTDEPTH,
    "org.khronos.openvx.convertdepth",
    vxConvertDepthKernel,
    convertdepth_kernel_params, dimof(convertdepth_kernel_params),
    NULL,
    vxConvertDepthInputValidator,
    vxConvertDepthOutputValidator,
    NULL,
    NULL,
};
