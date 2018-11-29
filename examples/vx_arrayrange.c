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

#include <VX/vx.h>

vx_array example_array_of_custom_type_and_initialization(vx_context context)
{
    //! [array define]
    typedef struct _mystruct {
        vx_uint32 some_uint;
        vx_float64 some_double;
    } mystruct;
#define MY_NUM_ITEMS (10)
    vx_enum mytype = vxRegisterUserStruct(context, sizeof(mystruct));
    vx_array array = vxCreateArray(context, mytype, MY_NUM_ITEMS);
    //! [array define]
    //! [array query]
    vx_size num_items = 0;
    vxQueryArray(array, VX_ARRAY_NUMITEMS, &num_items, sizeof(num_items));
    //! [array query]
    {
        //! [array range]
        vx_size i, stride = sizeof(vx_size);
        void *base = NULL;
        vx_map_id map_id;
        /* access entire array at once */
        vxMapArrayRange(array, 0, num_items, &map_id, &stride, &base, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
        for (i = 0; i < num_items; i++)
        {
            vxArrayItem(mystruct, base, i, stride).some_uint += i;
            vxArrayItem(mystruct, base, i, stride).some_double = 3.14f;
        }
        vxUnmapArrayRange(array, map_id);
        //! [array range]

        //! [array subaccess]
        /* access each array item individually */
        for (i = 0; i < num_items; i++)
        {
            mystruct *myptr = NULL;
            vxMapArrayRange(array, i, i+1, &map_id, &stride, (void **)&myptr, VX_READ_AND_WRITE, VX_MEMORY_TYPE_HOST, 0);
            myptr->some_uint += 1;
            myptr->some_double = 3.14f;
            vxUnmapArrayRange(array, map_id);
        }
        //! [array subaccess]
    }
    return array;
}
