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

#ifndef _OPENVX_INT_IMPORT_H_
#define _OPENVX_INT_IMPORT_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The Import Object Internal API.
 * \author Jesse Villarreal <jesse.villarreal@ti.com>
 *
 * \defgroup group_int_import Internal Import Object API
 * \ingroup group_internal
 * \brief The Internal Import Object API
 */

#if defined(OPENVX_USE_IX) || defined(OPENVX_USE_XML)


/*! \brief Create an import object.
 * \param [in] context The context.
 * \param [in] type The type of import.
 * \param [in] count The number of references to import.
 * \ingroup group_int_import
 */
vx_import ownCreateImportInt(vx_context context, vx_enum type, vx_uint32 count);

/*! \brief Destroys an Import and it's scoped-objects.
 *  \param [in] ref The import reference object.
 *  \ingroup group_int_import
 */
void ownDestructImport(vx_reference ref);

#endif

#endif
