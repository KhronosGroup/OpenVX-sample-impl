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

#ifndef _OPENVX_INT_REF_H_
#define _OPENVX_INT_REF_H_

#include <VX/vx.h>
#include "vx_internal.h"

/*!
 * \file
 * \brief The Internal Reference API
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_int_reference Internal Reference API
 * \ingroup group_internal
 * \brief The Internal Reference API
 */

/*! \brief Used to initialize any vx_reference.
 * \param [in] ref The pointer to the reference object.
 * \param [in] context The system context to put in the reference.
 * \param [in] type The type of the vx_<object>
 * \param [in] scope The scope under which the object is allocated.
 * \ingroup group_int_reference
 */
void ownInitReference(vx_reference ref, vx_context context, vx_enum type, vx_reference scope);

/*! \brief Used to initialize any vx_reference as a delay element
 * \param [in] ref The pointer to the reference object.
 * \param [in] d The delay to which the object belongs
 * \param [in] the index in the delay
 * \ingroup group_int_reference
 */
void ownInitReferenceForDelay(vx_reference ref, vx_delay_t *d, vx_int32 index);

/*! \brief Used to add a reference to the context.
 * \param [in] context The system context.
 * \param [in] ref The pointer to the reference object.
 * \ingroup group_int_reference
 */
vx_bool ownAddReference(vx_context context, vx_reference ref);

/*! \brief Used to create a reference.
 * \note This does not add the reference to the system context yet.
 * \param [in] context The system context.
 * \param [in] type The \ref vx_type_e type desired.
 * \ingroup group_int_reference
 */
vx_reference ownCreateReference(vx_context context, vx_enum type, vx_enum reftype, vx_reference scope);

/*! \brief Used to destroy an object in a generic way.
 * \ingroup group_int_reference
 */
typedef void (*vx_destructor_f)(vx_reference ref);

/*! \brief Allows the system to correlate the type with a known destructor.
 * \ingroup group_int_reference
 */
typedef struct vx_destructor_t {
    vx_enum type;
    vx_destructor_f destructor;
} vx_destructor_t;

/*! \brief Used to destroy a reference.
 * \param [in] ref The reference to release.
 * \param [in] type The \ref vx_type_e to check against.
 * \param [in] internal If true, the internal count is decremented, else the external
 * \param [in] special_destructor The a special function to call after the total count has reached zero, if NULL, a default destructor is used.
 * \ingroup group_int_reference
 */
vx_status ownReleaseReferenceInt(vx_reference *ref,
                        vx_enum type,
                        vx_enum reftype,
                        vx_destructor_f special_destructor);

/*! \brief Used to validate everything but vx_context, vx_image and vx_buffer.
 * \param [in] ref The reference to validate.
 * \ingroup group_implementation
 */
vx_bool ownIsValidReference(vx_reference ref);

/*! \brief Used to validate everything but vx_context, vx_image and vx_buffer.
 * \param [in] ref The reference to validate.
 * \param [in] type The \ref vx_type_e to check for.
 * \ingroup group_implementation
 */
vx_bool ownIsValidSpecificReference(vx_reference ref, vx_enum type);

/*! \brief Used to remove a reference from the context.
 * \param [in] context The system context.
 * \param [in] ref The pointer to the reference object.
 * \ingroup group_int_reference
 */
vx_bool ownRemoveReference(vx_context context, vx_reference ref);

/*! \brief Prints the values of a reference.
 * \param [in] ref The reference to print.
 * \ingroup group_int_reference
 */
void ownPrintReference(vx_reference ref);

/*! \brief Increments the ref count.
 * \param [in] ref The reference to print.
 * \ingroup group_int_reference
 */
vx_uint32 ownIncrementReference(vx_reference ref, vx_enum reftype);

/*! \brief Decrements the ref count.
 * \param [in] ref The reference to print.
 * \ingroup group_int_reference
 */
vx_uint32 ownDecrementReference(vx_reference ref, vx_enum reftype);

/*! \brief Returns the total reference count of the object.
 * \param [in] ref The reference to print.
 * \ingroup group_int_reference
 */
vx_uint32 ownTotalReferenceCount(vx_reference ref);

/*! \brief A tracking function used to increment the write usage counter and track
 * its side-effects.
 * \param [in] ref The reference to print.
 * \ingroup group_int_reference
 */
void ownWroteToReference(vx_reference ref);

/*! \brief A tracking function used to increment the read usage counter and track
 * its side-effects.
 * \param [in] ref The reference to print.
 * \ingroup group_int_reference
 */
void ownReadFromReference(vx_reference ref);

/*! \brief Returns the number of bytes in the internal structure for a given type.
 * \ingroup group_int_reference
 */
vx_size ownSizeOfType(vx_enum type);

#endif
