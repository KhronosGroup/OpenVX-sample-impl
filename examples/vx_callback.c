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

//! [callback]
#define MY_DESIRED_THRESHOLD (10)
vx_action VX_CALLBACK analyze_brightness(vx_node node) {
    // extract the max value
    vx_action action = VX_ACTION_ABANDON;
    vx_parameter pmax = vxGetParameterByIndex(node, 2); // Max Value
    if (pmax) {
        vx_scalar smax = 0;
        vxQueryParameter(pmax, VX_PARAMETER_REF, &smax, sizeof(smax));
        if (smax) {
            vx_uint8 value = 0u;
            vxCopyScalar(smax, &value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            if (value >= MY_DESIRED_THRESHOLD) {
                action = VX_ACTION_CONTINUE;
            }
            vxReleaseScalar(&smax);
        }
        vxReleaseParameter(&pmax);
    }
    return action;
}
//! [callback]

vx_status vx_example_callback(vx_context context, vx_image input, vx_image output) {
    (void)output;

    vx_status status = VX_SUCCESS;
    //! [graph setup]
    vx_graph graph = vxCreateGraph(context);
    status = vxGetStatus((vx_reference)graph);
    if (status == VX_SUCCESS) {
        vx_uint8 lmin = 0, lmax = 0;
        vx_uint32 minCount = 0, maxCount = 0;
        vx_scalar scalars[] = {
            vxCreateScalar(context, VX_TYPE_UINT8, &lmin),
            vxCreateScalar(context, VX_TYPE_UINT8, &lmax),
            vxCreateScalar(context, VX_TYPE_UINT32, &minCount),
            vxCreateScalar(context, VX_TYPE_UINT32, &maxCount),
        };
        vx_array arrays[] = {
            vxCreateArray(context, VX_TYPE_COORDINATES2D, 1),
            vxCreateArray(context, VX_TYPE_COORDINATES2D, 1)
        };
        vx_node nodes[] = {
            vxMinMaxLocNode(graph, input, scalars[0], scalars[1], arrays[0], arrays[1], scalars[2], scalars[3]),
            /// other nodes
        };
        status = vxAssignNodeCallback(nodes[0], &analyze_brightness);
        // do other
    }
    //! [graph setup]
    return status;
}
