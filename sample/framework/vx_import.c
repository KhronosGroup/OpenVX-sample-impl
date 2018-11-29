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

#include "vx_internal.h"
#include "vx_import.h"

#if defined(OPENVX_USE_XML) || defined(OPENVX_USE_IX)

vx_import ownCreateImportInt(vx_context context,
                              vx_enum type,
                              vx_uint32 count)
{
    vx_import import = NULL;

    if (ownIsValidContext(context) == vx_false_e)
        return 0;

    import = (vx_import)ownCreateReference(context, VX_TYPE_IMPORT, VX_EXTERNAL, &context->base);
    if (import && import->base.type == VX_TYPE_IMPORT)
    {
        import->refs = (vx_reference *)calloc(count, sizeof(vx_reference));
        import->type = type;
        import->count = count;
        VX_PRINT(VX_ZONE_INFO, "Creating Import of %u objects of type %x!\n", count, type);
    }
    return import;
}

void ownDestructImport(vx_reference ref) {
    vx_uint32 i = 0;
    vx_import import = (vx_import)ref;
    for (i = 0; i < import->count; i++)
    {
        ownReleaseReferenceInt(&import->refs[i], import->refs[i]->type, VX_INTERNAL, NULL);
    }
    if (import->refs) {
        free(import->refs);
    }
}

#endif

/******************************************************************************/
/* PUBLIC API */
/******************************************************************************/
#if defined(OPENVX_USE_XML)
VX_API_ENTRY vx_reference VX_API_CALL vxGetImportReferenceByIndex(vx_import import, vx_uint32 index)
{
    vx_reference ref = NULL;
    if (import && import->base.type == VX_TYPE_IMPORT)
    {
        if (index < import->count)
        {
            ref = (vx_reference_t *)import->refs[index];
            ownIncrementReference(ref, VX_EXTERNAL);
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Incorrect index value\n");
            vxAddLogEntry(&import->base.context->base, VX_ERROR_INVALID_PARAMETERS, "Incorrect index value\n");
            ref = (vx_reference_t *)ownGetErrorObject(import->base.context, VX_ERROR_INVALID_PARAMETERS);
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid import reference!\n");
    }
    return ref;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryImport(vx_import import, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidSpecificReference((vx_reference_t *)import, VX_TYPE_IMPORT) == vx_true_e)
    {
        switch (attribute)
        {
            case VX_IMPORT_ATTRIBUTE_COUNT:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3))
                {
                    *(vx_uint32 *)ptr = import->count;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_IMPORT_ATTRIBUTE_TYPE:
                if ((size <= VX_MAX_TARGET_NAME) && (ptr != NULL))
                {
                    *(vx_uint32 *)ptr = import->type;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }
    else
        status = VX_ERROR_INVALID_REFERENCE;
    return status;
}
#endif

#if defined(OPENVX_USE_IX) || defined(OPENVX_USE_XML)

VX_API_ENTRY vx_reference VX_API_CALL vxGetImportReferenceByName(vx_import import, const vx_char *name)
{
    vx_reference ref = NULL;
    if (import && import->base.type == VX_TYPE_IMPORT)
    {
        vx_uint32 index = 0;
        for (index = 0; index < import->count; index++)
        {
            if (strncmp(name, import->refs[index]->name, VX_MAX_REFERENCE_NAME) == 0)
            {
                ref = (vx_reference_t*)import->refs[index];
                ownIncrementReference(ref, VX_EXTERNAL);
                break;
            }
        }
    }
    return ref;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseImport(vx_import *import)
{
    return ownReleaseReferenceInt((vx_reference *)import, VX_TYPE_IMPORT, VX_EXTERNAL, NULL);
}

#endif