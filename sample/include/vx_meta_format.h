/* 

 * Copyright (c) 2014-2017 The Khronos Group Inc.
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


#ifndef _OPENVX_INT_META_FORMAT_H_
#define _OPENVX_INT_META_FORMAT_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The internal meta format implementation
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_meta_format The Meta-Format API
 * \ingroup group_internal
 * \brief The Internal Meta Format API
 */

/*! \brief Releases a meta-format object.
 * \param [in,out] pmeta
 * \ingroup group_int_meta_format
 */
void ownReleaseMetaFormat(vx_meta_format *pmeta);

/*! \brief Creates a metaformat object.
 * \param [in] context The overall context object.
 * \ingroup group_int_meta_format
 */
vx_meta_format ownCreateMetaFormat(vx_context context);

#endif
