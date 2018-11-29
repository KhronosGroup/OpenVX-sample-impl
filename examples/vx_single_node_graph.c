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
#include <VX/vxu.h>

vx_status vxSingleNodeGraph()
{
    vx_status status = VX_SUCCESS;
    vx_uint32 width = 320, height = 240;
    //! [whole]
    /* This is an example of a Single Node Graph */
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
    //! [node]
    vx_node node = vxGaussian3x3Node(graph, in, out);
    //! [node]

    //! [verify]
    status = vxVerifyGraph(graph);
    //! [verify]
    //! [process]
    if (status == VX_SUCCESS)
        status = vxProcessGraph(graph);
    //! [process]
    //! [whole]
    //! [vxu]
    status = vxuGaussian3x3(context, in, out);
    //! [vxu]

    //! [teardown]
    vxReleaseNode(&node);
    /* this also releases any nodes we forgot to release */
    vxReleaseGraph(&graph);
    vxReleaseImage(&in);
    vxReleaseImage(&out);
    /* this catches anything else we forgot to release */
    vxReleaseContext(&context);
    //! [teardown]
    return status;
}
