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

#ifndef _OPENVX_INT_MEMORY_H_
#define _OPENVX_INT_MEMORY_H_

#include <VX/vx.h>
#include "vx_internal.h"

/*! \file
 * \brief Defines an API for memory operations.
 *
 * \defgroup group_int_memory Internal Memory API.
 * \ingroup group_internal
 * \brief The Internal Memory API.
 */

/*! \brief Frees a memory block.
 * \ingroup group_int_memory
 */
vx_bool ownFreeMemory(vx_context_t *context, vx_memory_t *memory);

/*! \brief Allocates a memory block.
 * \ingroup group_int_memory
 */
vx_bool ownAllocateMemory(vx_context_t *context, vx_memory_t *memory);

void ownPrintMemory(vx_memory_t *mem);

vx_size ownComputeMemorySize(vx_memory_t *memory, vx_uint32 p);

#endif

