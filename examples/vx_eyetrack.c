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
 * \file vx_eyetrack.c
 * \example vx_eyetrack.c
 * \brief The example graph used to preprocess eye tracking data.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <vx_examples.h>

/*!
 * \brief An example of an Eye Tracking Graph.
 * \ingroup group_example
 */
int example_eyetracking(int argc, char *argv[])
{
    vx_status status = VX_SUCCESS;
    vx_uint32 width = 640;
    vx_uint32 height = 480;
    vx_bool running = vx_true_e;
    vx_context context = vxCreateContext();
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_image images[] = {
            vxCreateImage(context, width, height, VX_DF_IMAGE_UYVY),
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),
        };

        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_node nodes[] = {
                /*! \todo add nodes which process eye tracking */
            };

            status = vxVerifyGraph(context, graph);
            if (status == VX_SUCCESS)
            {
                do {
                    /*! \todo capture an image */
                    status = vxProcessGraph(context, graph, NULL);
                    /*! \todo do something with the data */
                } while (running == vx_true_e);
            }
            vxReleaseGraph(&graph);
        }
        vxReleaseContext(&context);
    }
    return status;
}

