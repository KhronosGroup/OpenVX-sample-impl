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

#ifndef _OPENVX_INT_ERROR_H_
#define _OPENVX_INT_ERROR_H_

#include <VX/vx.h>
#include "vx_internal.h"

/*!
 * \file
 * \brief The internal error implementation
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_error Internal Error API
 * \ingroup group_internal
 * \brief The Internal Error API.
 */

/*! \brief Creates all the status codes as error objects.
 * \ingroup group_int_error
 */
vx_bool ownCreateConstErrors(vx_context_t *context);

/*! \brief Releases an error object.
 * \ingroup group_int_error
 */
void ownReleaseErrorInt(vx_error_t **error);

/*! \brief Creates an Error Object.
 * \ingroup group_int_error
 */
vx_error_t *ownAllocateError(vx_context_t *context, vx_status status);

/*! \brief Matches the status code against all known error objects in the
 * context.
 * \param [in] context The pointer to the overall context.
 * \param [in] status The status code to find.
 * \return Returns a matching error object.
 */
vx_error_t *ownGetErrorObject(vx_context_t *context, vx_status status);

#endif

