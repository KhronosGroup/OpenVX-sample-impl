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
#include <stdlib.h>

#if __STDC_VERSION__ >= 199901L
#define OPENVX_USE_C99
#endif

vx_matrix example_random_matrix(vx_context context)
{
    //! [matrix]
    const vx_size columns = 3;
    const vx_size rows = 4;
    vx_matrix matrix = vxCreateMatrix(context, VX_TYPE_FLOAT32, columns, rows);
    vx_status status = vxGetStatus((vx_reference)matrix);
    if (status == VX_SUCCESS)
    {
        vx_int32 j, i;
#if defined(OPENVX_USE_C99)
        vx_float32 mat[rows][columns]; /* note: row major */
#else
        vx_float32 *mat = (vx_float32 *)malloc(rows*columns*sizeof(vx_float32));
#endif
        if (vxCopyMatrix(matrix, mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST) == VX_SUCCESS) {
            for (j = 0; j < (vx_int32)rows; j++)
                for (i = 0; i < (vx_int32)columns; i++)
#if defined(OPENVX_USE_C99)
                    mat[j][i] = (vx_float32)rand()/(vx_float32)RAND_MAX;
#else
                    mat[j*columns + i] = (vx_float32)rand()/(vx_float32)RAND_MAX;
#endif
            vxCopyMatrix(matrix, mat, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
        }
#if !defined(OPENVX_USE_C99)
        free(mat);
#endif
    }
    //! [matrix]
    return matrix;
}
