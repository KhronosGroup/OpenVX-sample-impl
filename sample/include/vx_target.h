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

#ifndef _OPENVX_INT_TARGET_H_
#define _OPENVX_INT_TARGET_H_

#include <VX/vx.h>
#include "vx_internal.h"

/*!
 * \file
 * \brief The internal target implementation.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_int_target Internal Target API
 * \ingroup group_internal
 * \brief The Internal Target API.
 */

/*! \brief Prints Target Information for Debugging.
 * \ingroup group_int_target
 */
void ownPrintTarget(vx_target_t *target, vx_uint32 index);

/*! \brief This allows the implementation to load a target interface into OpenVX.
 * \param [in] context The overall context pointer.
 * \param [in] name The shortened name of the target module.
 * \ingroup group_int_target
 */
vx_status ownLoadTarget(vx_context_t *context, vx_char *name);

/*! \brief This unloads a specific target in the targets list.
 * \param [in] context The overall context pointer.
 * \param [in] index The index into the context's target array.
 * \ingroup group_int_target
 */
vx_status ownUnloadTarget(vx_context_t *context, vx_uint32 index, vx_bool unload_module);

/*! \brief Initializes a target's kernels list.
 * \param [in] target The pointer to the target struct.
 * \param [in] kernels The array of kernels that the target supports.
 * \param [in] numkernels The length of the kernels list.
 * \ingroup group_int_target
 */
vx_status ownInitializeTarget(vx_target_t *target, vx_kernel_description_t *kernels[], vx_uint32 numkernels);

/*! \brief Deinitializes a target's kernels list.
 * \param [in] target The pointer to the target struct.
 * \param [in] kernels The array of kernels that the target supports.
 * \param [in] numkernels The length of the kernels list.
 * \ingroup group_int_target
 */
vx_status ownDeinitializeTarget(vx_target_t *target);

/*! \brief Match target name with specified target string.
 * \param [in] target_name The target name string.
 * \param [in] target_string The target string.
 * \ingroup group_int_target
 * \retval vx_true_e If string matches, vx_false_e if not.
 */
vx_bool ownMatchTargetNameWithString(const char* target_name, const char* target_string);

#endif
