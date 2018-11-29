/*

 * Copyright (c) 2013-2017 The Khronos Group Inc.
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
#include <VX/vx_lib_xyz.h>
#include <VX/vx_helper.h>

/*!
 * \file vx_xyz_lib.c
 * \example vx_xyz_lib.c
 * \brief An example of how to write a user mode extension to OpenVX.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */
//! [node]
vx_node vxXYZNode(vx_graph graph, vx_image input, vx_uint32 value, vx_image output, vx_array temp)
{
    vx_uint32 i;
    vx_node node = 0;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_status status = vxLoadKernels(context, "xyz");
    if (status == VX_SUCCESS)
    {
        //! [xyz node]
        vx_kernel kernel = vxGetKernelByName(context, VX_KERNEL_NAME_KHR_XYZ);
        if (kernel)
        {
            node = vxCreateGenericNode(graph, kernel);
            if (vxGetStatus((vx_reference)node) == VX_SUCCESS)
            {
                vx_status statuses[4];
                vx_scalar scalar = vxCreateScalar(context, VX_TYPE_INT32, &value);
                statuses[0] = vxSetParameterByIndex(node, 0, (vx_reference)input);
                statuses[1] = vxSetParameterByIndex(node, 1, (vx_reference)scalar);
                statuses[2] = vxSetParameterByIndex(node, 2, (vx_reference)output);
                statuses[3] = vxSetParameterByIndex(node, 3, (vx_reference)temp);
                vxReleaseScalar(&scalar);
                for (i = 0; i < dimof(statuses); i++)
                {
                    if (statuses[i] != VX_SUCCESS)
                    {
                        status = VX_ERROR_INVALID_PARAMETERS;
                        vxReleaseNode(&node);
                        vxReleaseKernel(&kernel);
                        node = 0;
                        kernel = 0;
                        break;
                    }
                }
            }
            else
            {
                vxReleaseKernel(&kernel);
            }
        }
        else
        {
            vxUnloadKernels(context, "xyz");
        }
        //! [xyz node]
    }
    return node;
}
//! [node]

//! [vxu]
vx_status vxuXYZ(vx_context context, vx_image input, vx_uint32 value, vx_image output, vx_array temp)
{
    vx_status status = VX_FAILURE;
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_node node = vxXYZNode(graph, input, value, output, temp);
        if (node)
        {
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
            }
            vxReleaseNode(&node);
        }
        vxReleaseGraph(&graph);
    }
    return status;
}
//! [vxu]
