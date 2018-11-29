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

#include <VX/vx.h>
#include <stdlib.h>

vx_status vx_example_introspection()
{
    //! [context]
    vx_context context = vxCreateContext();
    //! [context]
    //! [num]
    vx_size k, num_kernels = 0;
    vx_status status = vxQueryContext(context, VX_CONTEXT_UNIQUE_KERNELS, &num_kernels, sizeof(num_kernels));
    //! [num]
    if (num_kernels > 0) {
    //! [malloc]
    vx_size size = num_kernels * sizeof(vx_kernel_info_t);
    vx_kernel_info_t *table = (vx_kernel_info_t *)malloc(size);
    status = vxQueryContext(context, VX_CONTEXT_UNIQUE_KERNEL_TABLE, table, size);
    //! [malloc]

    //! [kernel_loop]
    for (k = 0; k < num_kernels; k++)
    //! [kernel_loop]
    {
        //! [kernel]
        vx_kernel kernel = vxGetKernelByName(context, table[k].name);
        //! [kernel]
        //! [kernel_attr]
        vx_uint32 p, num_params = 0u;
        status = vxQueryKernel(kernel, VX_KERNEL_PARAMETERS, &num_params, sizeof(num_params));
        //! [kernel_attr]
        if (status == VX_SUCCESS)
        {
            //! [parameter_loop]
            for (p = 0; p < num_params; p++)
            //! [parameter_loop]
            {
                //! [parameter]
                vx_parameter param = vxGetKernelParameterByIndex(kernel, p);
                //! [parameter]

                //! [parameter_attr]
                vx_enum dir, state, type;
                status = vxQueryParameter(param, VX_PARAMETER_DIRECTION, &dir, sizeof(dir));
                status = vxQueryParameter(param, VX_PARAMETER_STATE, &state, sizeof(state));
                status = vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(type));
                //! [parameter_attr]
            }
        }
        vxReleaseKernel(&kernel);
    }
    }
    //! [teardown]
    vxReleaseContext(&context);
    //! [teardown]
    return status;
}
