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

vx_status run_example_parallel(void)
{
    vx_status status = VX_SUCCESS;
    //! [independent]
    vx_context context = vxCreateContext();
    vx_image images[] = {
            vxCreateImage(context, 640, 480, VX_DF_IMAGE_UYVY),
            vxCreateImage(context, 640, 480, VX_DF_IMAGE_S16),
            vxCreateImage(context, 640, 480, VX_DF_IMAGE_U8),
    };
    vx_graph graph = vxCreateGraph(context);
    vx_image virts[] = {
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
    };

    vxChannelExtractNode(graph, images[0], VX_CHANNEL_Y, virts[0]),
    vxGaussian3x3Node(graph, virts[0], virts[1]),
    vxSobel3x3Node(graph, virts[1], virts[2], virts[3]),
    vxMagnitudeNode(graph, virts[2], virts[3], images[1]),
    vxPhaseNode(graph, virts[2], virts[3], images[2]),

    status = vxVerifyGraph(graph);
    if (status == VX_SUCCESS)
    {
        status = vxProcessGraph(graph);
    }
    vxReleaseContext(&context); /* this will release everything */
    //! [independent]
    return status;
 }
