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

#ifndef _OPENVX_INT_type_pairs_H_
#define _OPENVX_INT_type_pairs_H_

#include <stdint.h>
#include <VX/vx.h>

/*! \brief The enum string structure
 * \ingroup group_int_type_pairs
 */
typedef struct _vx_enum_string_t {
    /*! \brief The data type enumeration */
    vx_enum type;
    /*! \brief A character string to hold the name of the data type enum */
    vx_char name[64];
    /*! \brief Value of how many nibbles the data type uses */
    uintmax_t nibbles;
} vx_enum_string_t;

extern vx_enum_string_t type_pairs[];

vx_int32 ownStringFromType(vx_enum type);

#if defined (OPENVX_USE_XML)
vx_status ownTypeFromString(char *string, vx_enum *type);
vx_size ownMetaSizeOfType(vx_enum type);
#endif

#endif
