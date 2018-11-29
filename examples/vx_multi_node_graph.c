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
#include <VX/vx_helper.h>

vx_status vx_example_multinode_graph()
{
    vx_status status = VX_SUCCESS;
    vx_uint32 n, width = 320, height = 240;
    //! [context]
    vx_context context = vxCreateContext();
    //! [context]
    //! [data]
    vx_image in = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
    vx_image out = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
    //! [data]
    //! [graph]
    vx_graph graph = vxCreateGraph(context);
    //! [graph]
    //! [virt]
    vx_image blurred = vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT);
    vx_image gx = vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_S16);
    vx_image gy = vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_S16);
    //! [virt]
    //! [nodes]
    vx_node nodes[] = {
            vxGaussian3x3Node(graph, in, blurred),
            vxSobel3x3Node(graph, blurred, gx, gy),
            vxMagnitudeNode(graph, gx, gy, out),
    };
    //! [nodes]

    //! [verify]
    status = vxVerifyGraph(graph);
    //! [verify]

    //! [process]
    if (status == VX_SUCCESS)
        status = vxProcessGraph(graph);
    //! [process]

    //! [teardown]
    for (n = 0; n < dimof(nodes); n++)
        vxReleaseNode(&nodes[n]);
    /* this catches anything else we forgot to release */
    vxReleaseContext(&context);
    //! [teardown]
    return status;
}
