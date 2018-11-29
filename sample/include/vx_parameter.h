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

#ifndef _OPENVX_INT_PARAMETER_H_
#define _OPENVX_INT_PARAMETER_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The internal parameter implementation
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_parameter Internal Parameter API
 * \ingroup group_internal
 * \brief The Internal Parameter API
 */

/*! \brief This returns true if the direction is a valid enum
 * \param [in] dir The \ref vx_direction_e enum.
 * \ingroup group_int_parameter
 */
vx_bool ownIsValidDirection(vx_enum dir);

/*! \brief This returns true if the supplied type matches the expected type with
 * some fuzzy rules.
 * \ingroup group_int_parameter
 */
vx_bool ownIsValidTypeMatch(vx_enum expected, vx_enum supplied);

/*! \brief This returns true if the supplied state is a valid enum.
 * \ingroup group_int_parameter
 */
vx_bool ownIsValidState(vx_enum state);

/*! \brief Destroys a parameter.
 * \ingroup group_int_parameter
 */
void ownDestructParameter(vx_reference ref);

#endif
