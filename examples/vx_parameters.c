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

/*!
 * \file vx_parameters.c
 * \example vx_parameters.c
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

void example_explore_parameters(vx_context context, vx_kernel kernel)
{
    vx_uint32 p, numParams = 0;
    vx_graph graph = vxCreateGraph(context);
    vx_node node = vxCreateGenericNode(graph, kernel);
    vxQueryKernel(kernel, VX_KERNEL_PARAMETERS, &numParams, sizeof(numParams));
    for (p = 0; p < numParams; p++)
    {
        //! [Getting the ref]
        vx_parameter param = vxGetParameterByIndex(node, p);
        vx_reference ref;
        vxQueryParameter(param, VX_PARAMETER_REF, &ref, sizeof(ref));
        //! [Getting the ref]
        if (ref)
        {
            //! [Getting the type]
            vx_enum type;
            vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(type));
            /* cast the ref to the correct vx_<type>. Atomics are now vx_scalar */
            //! [Getting the type]
        }
        vxReleaseParameter(&param);
    }
    vxReleaseNode(&node);
    vxReleaseGraph(&graph);
}
