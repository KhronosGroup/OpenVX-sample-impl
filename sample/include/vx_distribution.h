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

#ifndef _OPENVX_INT_DISTRIBUTION_H_
#define _OPENVX_INT_DISTRIBUTION_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The internal distribution implementation
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_distribution Internal Distribution API
 * \ingroup group_internal
 * \brief The Internal Distribution API.
 */

/*! \brief Destroys a distribution.
 * \param [in] ref The generic handle to the object.
 * \ingroup group_int_distribution
 */
void ownDestructDistribution(vx_reference ref);

#endif
