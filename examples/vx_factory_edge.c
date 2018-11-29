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

#include <stdio.h>
#include <VX/vx.h>
#include <VX/vx_helper.h>

/*! \file vx_factory_edge.c
 * \example vx_factory_edge.c
 * \brief An example graph factory.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */


/*! \brief An example Graph Factory which creates an edge graph.
 * \ingroup group_example
 */
vx_graph vxEdgeGraphFactory(vx_context c)
{
    vx_status status = VX_FAILURE;
    vx_graph g = 0;
    if (c)
    {
        vx_kernel kernels[] = {
            vxGetKernelByEnum(c, VX_KERNEL_GAUSSIAN_3x3),
            vxGetKernelByEnum(c, VX_KERNEL_SOBEL_3x3),
            vxGetKernelByEnum(c, VX_KERNEL_MAGNITUDE),
        };

        g = vxCreateGraph(c);
        if (vxGetStatus((vx_reference)g) == VX_SUCCESS)
        {
            vx_image virts[] = {
                vxCreateVirtualImage(g, 0, 0, VX_DF_IMAGE_VIRT), // blurred
                vxCreateVirtualImage(g, 0, 0, VX_DF_IMAGE_VIRT), // gx
                vxCreateVirtualImage(g, 0, 0, VX_DF_IMAGE_VIRT), // gy
            };
            vx_node nodes[] = {
                vxCreateGenericNode(g, kernels[0]), // Gaussian
                vxCreateGenericNode(g, kernels[1]), // Sobel
                vxCreateGenericNode(g, kernels[2]), // Mag
            };
            vx_parameter params[] = {
                vxGetParameterByIndex(nodes[0], 0), // input Gaussian
                vxGetParameterByIndex(nodes[2], 2), // output Mag
            };
            vx_uint32 p = 0;
            status  = VX_SUCCESS;
            for (p = 0; p < dimof(params); p++)
            {
                // the parameter inherits from the node its attributes
                status |= vxAddParameterToGraph(g, params[p]);
            }
            // configure linkage of the graph
            status |= vxSetParameterByIndex(nodes[0], 1, (vx_reference)virts[0]);
            status |= vxSetParameterByIndex(nodes[1], 0, (vx_reference)virts[0]);
            status |= vxSetParameterByIndex(nodes[1], 1, (vx_reference)virts[1]);
            status |= vxSetParameterByIndex(nodes[1], 2, (vx_reference)virts[2]);
            status |= vxSetParameterByIndex(nodes[2], 0, (vx_reference)virts[1]);
            status |= vxSetParameterByIndex(nodes[2], 1, (vx_reference)virts[2]);
            for (p = 0; p < dimof(virts); p++)
            {
                /* this just decreases the external reference count */
                vxReleaseImage(&virts[p]);
            }
            if (status != VX_SUCCESS)
            {
                printf("Failed to create graph in factory!\n");
                vxReleaseGraph(&g);
                g = 0;
            }
        }
    }
    return g;
}

