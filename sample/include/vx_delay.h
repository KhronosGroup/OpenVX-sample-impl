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

#ifndef _OPENVX_INT_DELAY_H_
#define _OPENVX_INT_DELAY_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The Delay Object Internal API.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \author Amit Shoham <amit@bdti.com>
 *
 * \defgroup group_int_delay Internal Delay Object API
 * \ingroup group_internal
 * \brief The Internal Delay Object API
 */

/*! \brief Frees the delay object memory.
 * \param [in] delay The delay object.
 * \ingroup group_int_delay
 */
void ownFreeDelay(vx_delay delay);

/*! \brief Allocates a delay object's meta-data memory.
 * \param [in] count The number of objects in the delay.
 * \ingroup group_int_delay
 */
vx_delay ownAllocateDelay(vx_size count);

/*! \brief Gets a reference from delay object.
 * \param [in] delay The delay object.
 * \param [in] index The relative index desired.
 * \note Relative indexes are typically negative in delay objects to indicate
 * the historical nature.
 * \ingroup group_int_delay
 */
vx_reference ownGetRefFromDelay(vx_delay  delay, vx_int32 index);

/*! \brief Adds an association to a node to a delay slot object reference.
 * \param [in] value The delay slot object reference.
 * \param [in] n The node reference.
 * \param [in] i The index of the parameter.
 * \param [in] d The direction of the parameter.
 */
vx_bool ownAddAssociationToDelay(vx_reference value,
                                 vx_node n, vx_uint32 i);

/*! \brief Removes an association to a node from a delay slot object reference.
 * \param [in] value The delay slot object reference.
 * \param [in] n The node reference.
 * \param [in] i The index of the parameter.
 */
vx_bool ownRemoveAssociationToDelay(vx_reference value,
                                    vx_node n, vx_uint32 i);

/*! \brief Destroys a Delay and it's scoped-objects. */
void ownDestructDelay(vx_reference ref);

#endif
