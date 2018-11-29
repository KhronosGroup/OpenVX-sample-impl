/* 

 * Copyright (c) 2011-2017 The Khronos Group Inc.
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

#ifndef _OPENVX_INT_ARRAY_H_
#define _OPENVX_INT_ARRAY_H_

#include <VX/vx.h>

void ownPrintArray(vx_array array);

vx_array ownCreateArrayInt(vx_context context, vx_enum item_type, vx_size capacity, vx_bool is_virtual, vx_enum type);

void ownDestructArray(vx_reference reference);

void ownReleaseArrayInt(vx_array *array);

vx_bool ownInitVirtualArray(vx_array array, vx_enum item_type, vx_size capacity);

vx_bool ownValidateArray(vx_array array, vx_enum item_type, vx_size capacity);

vx_bool ownAllocateArray(vx_array array);

vx_status ownAccessArrayRangeInt(vx_array array, vx_size start, vx_size end, vx_size *pStride, void **ptr, vx_enum usage);
vx_status ownCommitArrayRangeInt(vx_array array, vx_size start, vx_size end, const void *ptr);

vx_status ownCopyArrayRangeInt(vx_array arr, vx_size start, vx_size end, vx_size stride, void *ptr, vx_enum usage, vx_enum mem_type);
vx_status ownMapArrayRangeInt(vx_array arr, vx_size start, vx_size end, vx_map_id *map_id, vx_size *stride,
                             void **ptr, vx_enum usage, vx_enum mem_type, vx_uint32 flags);
vx_status ownUnmapArrayRangeInt(vx_array arr, vx_map_id map_id);

#endif
