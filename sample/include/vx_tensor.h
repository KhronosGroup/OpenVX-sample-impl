/*

 * Copyright (c) 2016-2017 The Khronos Group Inc.
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

#ifndef _OPENVX_INT_TENSOR_H_
#define _OPENVX_INT_TENSOR_H_

#include <VX/vx.h>
#include "vx_internal.h"

/*!
 * \file
 * \brief The internal image implementation
 * \defgroup group_int_tensor Internal Tensor API
 * \ingroup group_internal
 * \brief The internal Tensor API.
 */


/*! \brief Used to validate the vx_tensor types.
 * \param [in] tennsor The vx_tensor to validate.
 * \ingroup group_int_tensor
 */
vx_bool ownIsValidTensor(vx_tensor tensor);

//TODO: missing documentation
void * ownAllocateTensorMemory(vx_tensor tensor);

/*! \brief Used to initialize the image meta-data structure with the correct
 * values per the df_image code.
 * \param [in,out] image The image object.
 * \param [in] width Width in pixels
 * \param [in] height Height in pixels
 * \param [in] color VX_DF_IMAGE color space.
 * \ingroup group_int_image
 */
void ownInitTensor(vx_tensor tensor, const vx_size* dimensions, vx_size number_of_dimensions, vx_enum data_type, vx_int8 fixed_point_position);

/*! \brief Used to free an image object.
 * \param [in] image The image object to free. Only the data is freed, not the
 * meta-data structure.
 * \ingroup group_int_image
 */
void ownFreeTensor(vx_tensor tensor);

/*! \brief Used to allocate an image object.
 * \param [in,out] image The image object.
 * \ingroup group_int_image
 */
void* ownAllocateTensorMemory(vx_tensor tensor);

/*! \brief Prints the values of the images.
 * \ingroup group_int_image
 */
void ownPrintTensor(vx_image image);

/*! \brief Destroys an image
 * \param [in] ref The image to destroy.
 * \ingroup group_int_image
 */
void ownDestructTensor(vx_reference ref);

#endif
