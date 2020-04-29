/*

 * Copyright (c) 2017-2017 The Khronos Group Inc.
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

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include <vx_internal.h>
#include <c_model.h>

static
vx_param_description_t copy_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_REFERENCE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_REFERENCE, VX_PARAMETER_STATE_REQUIRED }
};

static
vx_status VX_CALLBACK vxCopyKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (NULL != node && NULL != parameters && num == dimof(copy_kernel_params))
    {
        vx_reference input  = parameters[0];
        vx_reference output = parameters[1];
        status = vxCopy(input, output);
    }
    return status;
} /* vxCopyKernel() */

static
vx_status VX_CALLBACK vxCopyNodeValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(copy_kernel_params) && NULL != metas)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param))
        {
            vx_reference input = 0;

            status = vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(vx_reference));

            if (NULL != input)
                vxReleaseReference(&input);
        }

        if (NULL != param)
            vxReleaseParameter(&param);
    }
    return status;
} /* vxCopyNodeValidator() */

vx_kernel_description_t copy_kernel =
{
    VX_KERNEL_COPY,
    "org.khronos.openvx.copy",
    vxCopyKernel,
    copy_kernel_params, dimof(copy_kernel_params),
    vxCopyNodeValidator,
    NULL,
    NULL,
    NULL,
    NULL,
};
