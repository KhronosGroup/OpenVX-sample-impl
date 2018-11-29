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

void example_delaygraph(vx_context context)
{
    vx_status status = VX_SUCCESS;
    vx_image yuv = vxCreateImage(context, 320, 240, VX_DF_IMAGE_UYVY);
    vx_delay delay = vxCreateDelay(context, (vx_reference)yuv, 4);
    vx_image rgb = vxCreateImage(context, 320, 240, VX_DF_IMAGE_RGB);
    vx_graph graph = vxCreateGraph(context);

    vxColorConvertNode(graph, (vx_image)vxGetReferenceFromDelay(delay, 0),rgb);

    status = vxVerifyGraph(graph);
    if (status == VX_SUCCESS)
    {
        do {
             /* capture or read image into vxGetImageFromDelay(delay, 0); */
             status = vxProcessGraph(graph);
             /* 0 becomes -1, -1 becomes -2, etc. convert is updated with new 0 */
             vxAgeDelay(delay);
         } while (1);
    }
}
