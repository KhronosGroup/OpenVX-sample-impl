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

#ifndef _OPENVX_INT_PYRAMID_H_
#define _OPENVX_INT_PYRAMID_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The internal pyramid implementation
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_pyramid Internal Pyramid API
 * \ingroup group_internal
 * \brief The Internal Pyramid API.
 */

/*! \brief Releases a pyramid with internal references.
 * \param [in] pyramid The pyramid to release.
 * \ingroup group_int_pyramid
 */
void ownReleasePyramidInt(vx_pyramid pyramid);

/*! \brief Initializes the internals of a pyramid structure
 * \ingroup group_int_pyramid
 */
vx_status ownInitPyramid(vx_pyramid pyramid,
                        vx_size levels,
                        vx_float32 scale,
                        vx_uint32 width,
                        vx_uint32 height,
                        vx_df_image format);

/*! \brief Destroys a pyrmid object.
 * \ingroup group_int_pyramid
 */
void ownDestructPyramid(vx_reference ref);

#endif
