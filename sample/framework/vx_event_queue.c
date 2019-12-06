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

#ifdef OPENVX_USE_PIPELINING

#include <VX/vx.h>
#include <VX/vx_khr_pipelining.h>
#include <VX/vx_compatibility.h>

#include "vx_internal.h"

VX_API_ENTRY vx_status VX_API_CALL vxEnableEvents(vx_context context)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

VX_API_ENTRY vx_status VX_API_CALL vxDisableEvents(vx_context context)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

VX_API_ENTRY vx_status VX_API_CALL vxSendUserEvent(vx_context context, vx_uint32 id, void *parameter)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

VX_API_ENTRY vx_status VX_API_CALL vxWaitEvent(
                    vx_context context, vx_event_t *event,
                    vx_bool do_not_block)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

VX_API_ENTRY vx_status VX_API_CALL vxRegisterEvent(vx_reference ref,
                enum vx_event_type_e type, vx_uint32 param, vx_uint32 app_value)
{
    return VX_ERROR_NOT_IMPLEMENTED;
}

#endif

