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

#ifndef _OPENVX_INT_KERNEL_H_
#define _OPENVX_INT_KERNEL_H_

#include <VX/vx.h>
#include "vx_internal.h"

/*!
 * \file
 * \brief The internal kernel implementation.
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_kernel Internal Kernel API
 * \ingroup group_internal
 * \brief The internal Kernel API.
 */

/*! \brief Used to allocate a kernel object in the context.
 * \param [in] context The pointer to the context object.
 * \param [in] kenum The kernel enumeration value.
 * \param [in] function The pointer to the function of the kernel.
 * \param [in] name The name of the kernel in dotted notation.
 * \param [in] parameters The list of parameters for each kernel.
 * \param [in] numParams The number of parameters in the list.
 * \ingroup group_int_kernel
 */
vx_kernel_t *ownAllocateKernel(vx_context context,
                              vx_enum kenum,
                              vx_kernel_f function,
                              vx_char name[VX_MAX_KERNEL_NAME],
                              vx_param_description_t *parameters,
                              vx_uint32 numParams);

/*! \brief Used to initialize a kernel object in a target kernel list.
 * \param [in] context The pointer to the context object.
 * \param [in] kernel The pointer to the kernel structure.
 * \param [in] kenum The kernel enumeration value.
 * \param [in] function The pointer to the function of the kernel.
 * \param [in] name The name of the kernel in dotted notation.
 * \param [in] parameters The list of parameters for each kernel.
 * \param [in] numParams The number of parameters in the list.
 * \param [in] validator The function pointer to the params validator.
 * \param [in] in_validator The function pointer to the input validator.
 * \param [in] out_validator The function pointer to the output validator.
 * \param [in] initialize The function to call to initialize the kernel.
 * \param [in] deinitialize The function to call to deinitialize the kernel.
 * \ingroup group_int_kernel
 */
vx_status ownInitializeKernel(vx_context context,
                             vx_kernel kernel,
                             vx_enum kenum,
                             vx_kernel_f function,
                             vx_char name[VX_MAX_KERNEL_NAME],
                             vx_param_description_t *parameters,
                             vx_uint32 numParams,
                             vx_kernel_validate_f validator,
                             vx_kernel_input_validate_f in_validator,
                             vx_kernel_output_validate_f out_validator,
                             vx_kernel_initialize_f initialize,
                             vx_kernel_deinitialize_f deinitialize);

/*! \brief Used to deinitialize a kernel object in a target kernel list.
 * \param [in] kernel The pointer to the kernel structure.
 * \ingroup group_int_kernel
 */
vx_status ownDeinitializeKernel(vx_kernel *kernel);

/*! \brief Determines if a kernel is unique in the system.
 * \param kernel The handle to the kernel.
 * \ingroup group_int_kernel
 */
vx_bool ownIsKernelUnique(vx_kernel kernel);

#endif
