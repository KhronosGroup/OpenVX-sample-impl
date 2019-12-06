/*

 * Copyright (c) 2012-2019 The Khronos Group Inc.
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

#ifdef OPENVX_USE_STREAMING

#include <VX/vx.h>
#include <VX/vx_khr_pipelining.h>
#include <VX/vx_compatibility.h>

#include "vx_internal.h"

VX_API_ENTRY vx_status VX_API_CALL vxEnableGraphStreaming(vx_graph graph,
                vx_node trigger_node)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

VX_API_ENTRY vx_status VX_API_CALL vxStartGraphStreaming(vx_graph graph)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

VX_API_ENTRY vx_status VX_API_CALL vxStopGraphStreaming(vx_graph graph)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

#endif
