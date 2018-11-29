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
#include <stdlib.h>
#include <VX/vx.h>
#include "VX/vx_khr_nn.h"
#include "tensor_utils.h"
vx_size ComputeGlobalPositionsFromIndex(vx_size index, const vx_size * dimensions,
		const vx_size * stride, vx_size number_of_dimensions, vx_size * pos)
{
	*pos = 0;
	vx_size index_leftover = index;
	int divisor = 1;
	for (vx_size i = 0; i < number_of_dimensions; i++)
	{
		divisor = (int)dimensions[i];
		vx_size curr_dim_index = index_leftover%divisor;
		*pos += stride[i] * curr_dim_index ;
		index_leftover = index_leftover /divisor;
	}

    return *pos;
}
vx_size ComputeNumberOfElements (const vx_size * dimensions, vx_size number_of_dimensions)
{
	vx_size total_size = 1;
	for (vx_size i = 0; i < number_of_dimensions; i++)
	{
		total_size *= dimensions[i];
	}
	return total_size;
}

vx_status AllocatePatch (vx_tensor tensor, vx_size* dims_num, vx_size* dimensions, vx_size* stride, void** buffer_ptr, vx_enum usage)  {
	vx_status status = VX_SUCCESS;
	vx_size view_start[MAX_NUM_OF_DIMENSIONS] = {0};
	vx_enum format;
	status |= vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE, &format, sizeof(format));
	status |= vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, dims_num, sizeof(*dims_num));
	status |= vxQueryTensor(tensor, VX_TENSOR_DIMS, dimensions, sizeof(vx_size) * (*dims_num));
	switch(format)
	{
	case VX_TYPE_INT16: stride[0] = sizeof(vx_int16); break;
    case VX_TYPE_INT8: stride[0] = sizeof(vx_int8); break;
    case VX_TYPE_UINT8: stride[0] = sizeof(vx_uint8); break;

	}

    for (vx_size i = 1; i < *dims_num; i++) {
		stride[i] = dimensions[i-1] * stride[i-1];
	}
	*buffer_ptr = calloc(dimensions[*dims_num - 1]*stride[*dims_num - 1], 1);
	if (*buffer_ptr)
	{
		if (usage == VX_READ_ONLY)
			status |= vxCopyTensorPatch(tensor, *dims_num, view_start, dimensions, stride, *buffer_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
		else
			status = VX_SUCCESS;
	}
	else
		status = VX_ERROR_NO_MEMORY;
	return status;
}
vx_status ReleasePatch (vx_tensor tensor, vx_size dims_num, const vx_size* dimensions, const vx_size* stride, void** buffer_ptr, vx_enum usage) {
	vx_status status = VX_SUCCESS;
	vx_size view_start[MAX_NUM_OF_DIMENSIONS] = {0};
	if (*buffer_ptr )
	{
		if (usage == VX_WRITE_ONLY)
			status |= vxCopyTensorPatch(tensor, dims_num, view_start, dimensions, stride, *buffer_ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
	}
	else
		status = VX_ERROR_NOT_ALLOCATED;
	free(*buffer_ptr);
	return status;
}
