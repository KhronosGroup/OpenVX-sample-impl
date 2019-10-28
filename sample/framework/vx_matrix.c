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
#include "vx_matrix.h"

void ownDestructMatrix(vx_reference ref)
{
    vx_matrix matrix = (vx_matrix)ref;
    ownFreeMemory(matrix->base.context, &matrix->memory);
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseMatrix(vx_matrix *mat)
{
    return ownReleaseReferenceInt((vx_reference *)mat, VX_TYPE_MATRIX, VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_matrix VX_API_CALL vxCreateMatrix(vx_context context, vx_enum data_type, vx_size columns, vx_size rows)
{
    vx_matrix matrix = NULL;
    vx_size dim = 0ul;

    if (ownIsValidContext(context) == vx_false_e)
        return 0;

    if ((data_type == VX_TYPE_INT8) || (data_type == VX_TYPE_UINT8))
    {
        dim = sizeof(vx_uint8);
    }
    else if ((data_type == VX_TYPE_INT16) || (data_type == VX_TYPE_UINT16))
    {
        dim = sizeof(vx_uint16);
    }
    else if ((data_type == VX_TYPE_INT32) || (data_type == VX_TYPE_UINT32) || (data_type == VX_TYPE_FLOAT32))
    {
        dim = sizeof(vx_uint32);
    }
    else if ((data_type == VX_TYPE_INT64) || (data_type == VX_TYPE_UINT64) || (data_type == VX_TYPE_FLOAT64))
    {
        dim = sizeof(vx_uint64);
    }
    if (dim == 0ul)
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid data type\n");
        vxAddLogEntry(&context->base, VX_ERROR_INVALID_TYPE, "Invalid data type\n");
        return (vx_matrix)ownGetErrorObject(context, VX_ERROR_INVALID_TYPE);
    }
    if ((columns == 0ul) || (rows == 0ul))
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid dimensions to matrix\n");
        vxAddLogEntry(&context->base, VX_ERROR_INVALID_DIMENSION, "Invalid dimensions to matrix\n");
        return (vx_matrix)ownGetErrorObject(context, VX_ERROR_INVALID_DIMENSION);
    }
    matrix = (vx_matrix)ownCreateReference(context, VX_TYPE_MATRIX, VX_EXTERNAL, &context->base);
    if (vxGetStatus((vx_reference)matrix) == VX_SUCCESS && matrix->base.type == VX_TYPE_MATRIX)
    {
        matrix->data_type = data_type;
        matrix->columns = columns;
        matrix->rows = rows;
        matrix->memory.ndims = 2;
        matrix->memory.nptrs = 1;
        matrix->memory.dims[0][0] = (vx_uint32)dim;
        matrix->memory.dims[0][1] = (vx_uint32)(columns*rows);
        matrix->origin.x = (vx_uint32)(columns / 2);
        matrix->origin.y = (vx_uint32)(rows / 2);
        matrix->pattern = VX_PATTERN_OTHER;
    }
    return (vx_matrix)matrix;
}

VX_API_ENTRY vx_matrix VX_API_CALL vxCreateVirtualMatrix(vx_graph graph, vx_enum data_type, vx_size columns, vx_size rows)
{
    vx_matrix matrix = NULL;
    vx_reference_t *gref = (vx_reference_t *)graph;

    if (ownIsValidSpecificReference(gref, VX_TYPE_GRAPH) == vx_true_e)
    {
        matrix = vxCreateMatrix(gref->context, data_type, columns, rows);
        if (vxGetStatus((vx_reference)matrix) == VX_SUCCESS && matrix->base.type == VX_TYPE_MATRIX)
        {
            matrix->base.scope = (vx_reference_t *)graph;
            matrix->base.is_virtual = vx_true_e;
        }
    }

    return (vx_matrix)matrix;
}


VX_API_ENTRY vx_matrix VX_API_CALL vxCreateMatrixFromPattern(vx_context context, vx_enum pattern, vx_size columns, vx_size rows)
{
    if (ownIsValidContext(context) == vx_false_e)
        return 0;

    if ((columns > VX_INT_MAX_NONLINEAR_DIM) || (rows > VX_INT_MAX_NONLINEAR_DIM))
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid dimensions to matrix\n");
        vxAddLogEntry(&context->base, VX_ERROR_INVALID_DIMENSION, "Invalid dimensions to matrix\n");
        return (vx_matrix)ownGetErrorObject(context, VX_ERROR_INVALID_DIMENSION);
    }

    vx_matrix matrix = vxCreateMatrix(context, VX_TYPE_UINT8, columns, rows);
    if (ownIsValidSpecificReference(&matrix->base, VX_TYPE_MATRIX) == vx_true_e)
    {
        if (ownAllocateMemory(matrix->base.context, &matrix->memory) == vx_true_e)
        {
            ownSemWait(&matrix->base.lock);
            vx_uint8* ptr = matrix->memory.ptrs[0];
            vx_size x, y;
            for (y = 0; y < rows; ++y)
            {
                for (x = 0; x < columns; ++x)
                {
                    vx_uint8 value = 0;
                    switch (pattern)
                    {
                    case VX_PATTERN_BOX: value = 255; break;
                    case VX_PATTERN_CROSS: value = ((y == rows / 2) || (x == columns / 2)) ? 255 : 0; break;
                    case VX_PATTERN_DISK:
                        value = (((y - rows / 2.0 + 0.5) * (y - rows / 2.0 + 0.5)) / ((rows / 2.0) * (rows / 2.0)) +
                            ((x - columns / 2.0 + 0.5) * (x - columns / 2.0 + 0.5)) / ((columns / 2.0) * (columns / 2.0)))
                            <= 1 ? 255 : 0;
                        break;
                    }
                    ptr[x + y * columns] = value;
                }
            }

            ownSemPost(&matrix->base.lock);
            ownWroteToReference(&matrix->base);
            matrix->pattern = pattern;
        }
        else
        {
            vxReleaseMatrix(&matrix);
            VX_PRINT(VX_ZONE_ERROR, "Failed to allocate matrix\n");
            vxAddLogEntry(&context->base, VX_ERROR_NO_MEMORY, "Failed to allocate matrix\n");
            matrix = (vx_matrix)ownGetErrorObject(context, VX_ERROR_NO_MEMORY);
        }
    }

    return matrix;
}

VX_API_ENTRY vx_matrix VX_API_CALL vxCreateMatrixFromPatternAndOrigin(vx_context context, vx_enum pattern, vx_size columns, vx_size rows, vx_size origin_col, vx_size origin_row)
{
    vx_matrix matrix = vxCreateMatrixFromPattern(context, pattern, columns, rows);
    if (ownIsValidSpecificReference(&matrix->base, VX_TYPE_MATRIX) == vx_true_e)
    {
        if (vxGetStatus((vx_reference)matrix) == VX_SUCCESS && matrix->base.type == VX_TYPE_MATRIX)
        {
            if(origin_col < columns && origin_row < rows)
            {
                matrix->origin.x = (vx_uint32)origin_col;
                matrix->origin.y = (vx_uint32)origin_row;
            }
            else
            {
                vxReleaseMatrix(&matrix);
                VX_PRINT(VX_ZONE_ERROR, "Origin point error\n");
                vxAddLogEntry(&context->base, VX_ERROR_INVALID_PARAMETERS, "Origin point error\n");
                matrix = (vx_matrix)ownGetErrorObject(context, VX_ERROR_INVALID_PARAMETERS);
            }
        }
    }
    return matrix;
}
VX_API_ENTRY vx_status VX_API_CALL vxQueryMatrix(vx_matrix matrix, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;
    if (ownIsValidSpecificReference(&matrix->base, VX_TYPE_MATRIX) == vx_false_e)
    {
        return VX_ERROR_INVALID_REFERENCE;
    }
    switch (attribute)
    {
        case VX_MATRIX_TYPE:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
            {
                *(vx_enum *)ptr = matrix->data_type;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_MATRIX_ROWS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
            {
                *(vx_size *)ptr = matrix->rows;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_MATRIX_COLUMNS:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
            {
                *(vx_size *)ptr = matrix->columns;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_MATRIX_SIZE:
            if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
            {
                *(vx_size *)ptr = matrix->columns * matrix->rows * matrix->memory.dims[0][0];
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_MATRIX_ORIGIN:
            if (VX_CHECK_PARAM(ptr, size, vx_coordinates2d_t, 0x3))
            {
                *(vx_coordinates2d_t*)ptr = matrix->origin;
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            break;
        case VX_MATRIX_PATTERN:
            if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
            {
                *(vx_enum*)ptr = matrix->pattern;
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
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxReadMatrix(vx_matrix matrix, void *array)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (ownIsValidSpecificReference(&matrix->base, VX_TYPE_MATRIX) == vx_true_e)
    {
        if (ownAllocateMemory(matrix->base.context, &matrix->memory) == vx_true_e)
        {
            ownSemWait(&matrix->base.lock);
            if (array)
            {
                vx_size size = matrix->memory.strides[0][1] *
                               matrix->memory.dims[0][1];
                memcpy(array, matrix->memory.ptrs[0], size);
            }
            ownSemPost(&matrix->base.lock);
            ownReadFromReference(&matrix->base);
            status = VX_SUCCESS;
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Failed to allocate matrix\n");
            status = VX_ERROR_NO_MEMORY;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid reference for matrix\n");
    }
    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxWriteMatrix(vx_matrix matrix, const void *array)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    if (ownIsValidSpecificReference(&matrix->base, VX_TYPE_MATRIX) == vx_true_e)
    {
        if (ownAllocateMemory(matrix->base.context, &matrix->memory) == vx_true_e)
        {
            ownSemWait(&matrix->base.lock);
            if (array)
            {
                vx_size size = matrix->memory.strides[0][1] *
                               matrix->memory.dims[0][1];
                memcpy(matrix->memory.ptrs[0], array, size);
            }
            ownSemPost(&matrix->base.lock);
            ownWroteToReference(&matrix->base);
            status = VX_SUCCESS;
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Failed to allocate matrix\n");
            status = VX_ERROR_NO_MEMORY;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid reference for matrix\n");
    }
    return status;
}

vx_status VX_API_CALL vxCopyMatrix(vx_matrix matrix, void *ptr, vx_enum usage, vx_enum mem_type)
{
    vx_status status = VX_ERROR_INVALID_REFERENCE;
    (void)mem_type;

    if (ownIsValidSpecificReference(&matrix->base, VX_TYPE_MATRIX) == vx_true_e)
    {
        if (ownAllocateMemory(matrix->base.context, &matrix->memory) == vx_true_e)
        {
#ifdef OPENVX_USE_OPENCL_INTEROP
            void * ptr_given = ptr;
            vx_enum mem_type_given = mem_type;
            if (mem_type == VX_MEMORY_TYPE_OPENCL_BUFFER)
            {
                // get ptr from OpenCL buffer for HOST
                size_t size = 0;
                cl_mem opencl_buf = (cl_mem)ptr;
                cl_int cerr = clGetMemObjectInfo(opencl_buf, CL_MEM_SIZE, sizeof(size_t), &size, NULL);
                VX_PRINT(VX_ZONE_CONTEXT, "OPENCL: vxCopyMatrix: clGetMemObjectInfo(%p) => (%d)\n",
                    opencl_buf, cerr);
                if (cerr != CL_SUCCESS)
                {
                    return VX_ERROR_INVALID_PARAMETERS;
                }
                ptr = clEnqueueMapBuffer(matrix->base.context->opencl_command_queue,
                    opencl_buf, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, size,
                    0, NULL, NULL, &cerr);
                VX_PRINT(VX_ZONE_CONTEXT, "OPENCL: vxCopyMatrix: clEnqueueMapBuffer(%p,%d) => %p (%d)\n",
                    opencl_buf, (int)size, ptr, cerr);
                if (cerr != CL_SUCCESS)
                {
                    return VX_ERROR_INVALID_PARAMETERS;
                }
                mem_type = VX_MEMORY_TYPE_HOST;
            }
#endif
            if (usage == VX_READ_ONLY)
            {
                ownSemWait(&matrix->base.lock);
                if (ptr)
                {
                    vx_size size = matrix->memory.strides[0][1] *
                                   matrix->memory.dims[0][1];
                    memcpy(ptr, matrix->memory.ptrs[0], size);
                }
                ownSemPost(&matrix->base.lock);
                ownReadFromReference(&matrix->base);
                status = VX_SUCCESS;
            }
            else if (usage == VX_WRITE_ONLY)
            {
                ownSemWait(&matrix->base.lock);
                if (ptr)
                {
                    vx_size size = matrix->memory.strides[0][1] *
                                   matrix->memory.dims[0][1];
                    memcpy(matrix->memory.ptrs[0], ptr, size);
                }
                ownSemPost(&matrix->base.lock);
                ownWroteToReference(&matrix->base);
                status = VX_SUCCESS;
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "Wrong parameters for matrix\n");
                status = VX_ERROR_INVALID_PARAMETERS;
            }

#ifdef OPENVX_USE_OPENCL_INTEROP
            if (mem_type_given == VX_MEMORY_TYPE_OPENCL_BUFFER)
            {
                clEnqueueUnmapMemObject(matrix->base.context->opencl_command_queue,
                    (cl_mem)ptr_given, ptr, 0, NULL, NULL);
                clFinish(matrix->base.context->opencl_command_queue);
            }
#endif
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Failed to allocate matrix\n");
            status = VX_ERROR_NO_MEMORY;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid reference for matrix\n");
    }
    return status;
}
