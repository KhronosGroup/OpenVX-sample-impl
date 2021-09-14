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
 * \brief The Extra Extensions Module
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>

#include "vx_extras_module.h"

/*! \brief Declares the list of all supported base kernels.
 * \ingroup group_debug_ext
 */
static vx_kernel_description_t* kernels[] =
{
    &edge_trace_kernel,
    &euclidean_nonmaxsuppression_harris_kernel,
    &harris_score_kernel,
    &laplacian3x3_kernel,
    &image_lister_kernel,
    &nonmaxsuppression_kernel,
    &norm_kernel,
    &scharr3x3_kernel,
    &sobelMxN_kernel,
};

/*! \brief Declares the number of base supported kernels.
 * \ingroup group_implementation
 */
static vx_uint32 num_kernels = dimof(kernels);

//**********************************************************************
//  PUBLIC FUNCTION
//**********************************************************************

/*! \brief The entry point into this module to add the base kernels to OpenVX.
 * \param context The handle to the implementation context.
 * \return vx_status Returns errors if some or all kernels were not added
 * correctly.
 * \ingroup group_implementation
 */
/*VX_API_ENTRY*/ vx_status VX_API_CALL vxPublishKernels(vx_context context)
{
    vx_status status = VX_FAILURE;
    vx_uint32 p = 0, k = 0;
    for (k = 0; k < num_kernels; k++)
    {
        vx_kernel kernel = 0;

        kernel = vxAddUserKernel(context,
                     kernels[k]->name,
                     kernels[k]->enumeration,
                     kernels[k]->function,
                     kernels[k]->numParams,
                     kernels[k]->validate,
                     kernels[k]->initialize,
                     kernels[k]->deinitialize);
        if (kernel)
        {
            status = VX_SUCCESS; // temporary
            for (p = 0; p < kernels[k]->numParams; p++)
            {
                status = vxAddParameterToKernel(kernel, p,
                                                kernels[k]->parameters[p].direction,
                                                kernels[k]->parameters[p].data_type,
                                                kernels[k]->parameters[p].state);
                if (status != VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)context, status, "Failed to add parameter %d to kernel %s! (%d)\n", p, kernels[k]->name, status);
                    break;
                }
            }
            if (status == VX_SUCCESS)
            {
                status = vxFinalizeKernel(kernel);
                if (status != VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)context, status, "Failed to finalize kernel[%u]=%s\n",k, kernels[k]->name);
                }
            }
            else
            {
                status = vxRemoveKernel(kernel);
                if (status != VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)context, status, "Failed to remove kernel[%u]=%s\n",k, kernels[k]->name);
                }
            }
        }
        else
        {
            vxAddLogEntry((vx_reference)context, status, "Failed to add kernel %s\n", kernels[k]->name);
        }
    }
    return status;
}

/*! \brief The destructor to remove a user loaded module from OpenVX.
 * \param [in] context The handle to the implementation context.
 * \return A \ref vx_status_e enumeration. Returns errors if some or all kernels were not added
 * correctly.
 * \note This follows the function pointer definition of a \ref vx_unpublish_kernels_f
 * and uses the predefined name for the entry point, "vxUnpublishKernels".
 * \ingroup group_example_kernel
 */
/*VX_API_ENTRY*/ vx_status VX_API_CALL vxUnpublishKernels(vx_context context)
{
    vx_status status = VX_FAILURE;

    vx_uint32 k = 0;
    for (k = 0; k < num_kernels; k++)
    {
        vx_kernel kernel = vxGetKernelByName(context, kernels[k]->name);
        vx_kernel kernelcpy = kernel;

        if (kernel)
        {
            status = vxReleaseKernel(&kernelcpy);
            if (status != VX_SUCCESS)
            {
                vxAddLogEntry((vx_reference)context, status, "Failed to release kernel[%u]=%s\n",k, kernels[k]->name);
            }
            else
            {
                kernelcpy = kernel;
                status = vxRemoveKernel(kernelcpy);
                if (status != VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)context, status, "Failed to remove kernel[%u]=%s\n",k, kernels[k]->name);
                }
            }
        }
        else
        {
            vxAddLogEntry((vx_reference)context, status, "Failed to get added kernel %s\n", kernels[k]->name);
        }
    }

    return status;
}
