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
#include "vx_type_pairs.h"

#if defined(EXPERIMENTAL_USE_DOT) || defined(OPENVX_USE_XML)

#include "vx_type_pairs.h"

vx_enum_string_t type_pairs[] = {
    {VX_STRINGERIZE(VX_TYPE_INVALID),0},
    /* scalar objects */
    {VX_STRINGERIZE(VX_TYPE_CHAR),sizeof(vx_char)*2},
    {VX_STRINGERIZE(VX_TYPE_UINT8),sizeof(vx_uint8)*2},
    {VX_STRINGERIZE(VX_TYPE_UINT16),sizeof(vx_uint16)*2},
    {VX_STRINGERIZE(VX_TYPE_UINT32),sizeof(vx_uint32)*2},
    {VX_STRINGERIZE(VX_TYPE_UINT64),sizeof(vx_uint64)*2},
    {VX_STRINGERIZE(VX_TYPE_INT8),sizeof(vx_int8)*2},
    {VX_STRINGERIZE(VX_TYPE_INT16),sizeof(vx_int16)*2},
    {VX_STRINGERIZE(VX_TYPE_INT32),sizeof(vx_int32)*2},
    {VX_STRINGERIZE(VX_TYPE_INT64),sizeof(vx_int64)*2},
    {VX_STRINGERIZE(VX_TYPE_FLOAT32),sizeof(vx_float32)*2},
    {VX_STRINGERIZE(VX_TYPE_FLOAT64),sizeof(vx_float64)*2},
    {VX_STRINGERIZE(VX_TYPE_SIZE),sizeof(vx_size)*2},
    {VX_STRINGERIZE(VX_TYPE_DF_IMAGE),sizeof(vx_df_image)*2},
    {VX_STRINGERIZE(VX_TYPE_BOOL),sizeof(vx_bool)*2},
    {VX_STRINGERIZE(VX_TYPE_ENUM),sizeof(vx_enum)*2},
    /* struct objects */
    {VX_STRINGERIZE(VX_TYPE_COORDINATES2D),sizeof(vx_coordinates2d_t)*2},
    {VX_STRINGERIZE(VX_TYPE_COORDINATES3D),sizeof(vx_coordinates3d_t)*2},
    {VX_STRINGERIZE(VX_TYPE_RECTANGLE),sizeof(vx_rectangle_t)*2},
    {VX_STRINGERIZE(VX_TYPE_KEYPOINT),sizeof(vx_keypoint_t)*2},
    /* data objects */
    {VX_STRINGERIZE(VX_TYPE_ARRAY),0},
    {VX_STRINGERIZE(VX_TYPE_DISTRIBUTION),0},
    {VX_STRINGERIZE(VX_TYPE_LUT),0},
    {VX_STRINGERIZE(VX_TYPE_IMAGE),0},
    {VX_STRINGERIZE(VX_TYPE_CONVOLUTION),0},
    {VX_STRINGERIZE(VX_TYPE_THRESHOLD),0},
    {VX_STRINGERIZE(VX_TYPE_MATRIX),0},
    {VX_STRINGERIZE(VX_TYPE_SCALAR),0},
    {VX_STRINGERIZE(VX_TYPE_PYRAMID),0},
    {VX_STRINGERIZE(VX_TYPE_REMAP),0},
#ifdef OPENVX_KHR_XML
    {VX_STRINGERIZE(VX_TYPE_IMPORT),0},
#endif
};

vx_int32 ownStringFromType(vx_enum type)
{
    vx_uint32 i = 0u;
    for (i = 0u; i < dimof(type_pairs); i++)
    {
        if (type == type_pairs[i].type)
        {
            return i;
        }
    }
    return -1;
}

#endif

#if defined(OPENVX_USE_XML)

vx_status ownTypeFromString(char *string, vx_enum *type)
{
    vx_status status = VX_ERROR_INVALID_TYPE;
    vx_uint32 i = 0u;

    for (i = 0u; i < dimof(type_pairs); i++)
    {
        if (strncmp(string, type_pairs[i].name, sizeof(type_pairs[i].name)) == 0)
        {
            *type = type_pairs[i].type;
            status = VX_SUCCESS;
            break;
        }
    }
    return status;
}

vx_size ownMetaSizeOfType(vx_enum type)
{
    vx_size size = 0ul;
    vx_uint32 i = 0u;
    for (i = 0u; i < dimof(type_pairs); i++)
    {
        if (type_pairs[i].type == type)
        {
            size = type_pairs[i].nibbles / 2;
            break;
        }
    }
    return size;
}

#endif
