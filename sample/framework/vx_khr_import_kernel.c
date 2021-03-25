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

#ifdef OPENVX_CONFORMANCE_NNEF_IMPORT

#include <vx_kernel.h>

#include "cnnef.h"

#define MAXLEN 512

static inline size_t sizeof_tensor_type(vx_int32 type)
{
    if (type == VX_TYPE_FLOAT32)
        return sizeof(vx_float32);;
    if (type == VX_TYPE_INT32)
        return sizeof(vx_int32);
    if (type == VX_TYPE_BOOL)
        return sizeof(vx_bool);

    return 0;
}

static inline enum vx_type_e tensor_vx_type(const vx_char * nnef_dtype)
{
    if (strcmp(nnef_dtype, "scalar") == 0)
    {
        return VX_TYPE_FLOAT32;
    }
    else if (strcmp(nnef_dtype, "integer") == 0)
    {
        return VX_TYPE_INT32;
    }
    else if (strcmp(nnef_dtype, "logical") == 0)
    {
        return VX_TYPE_BOOL;
    }
    return VX_TYPE_INVALID;
}

static inline vx_size compute_patch_size(const vx_size *view_end, vx_size number_of_dimensions)
{
    vx_size total_size = 1;
    for (vx_size i = 0; i < number_of_dimensions; i++)
    {
        total_size *= view_end[i];
    }
    return total_size;
}

// copying the dims into dimensions since OpenVX tensor created requires size_t
static inline void copy_dims(const vx_int32 *dims, vx_int32 num_dims, vx_size *dimensions)
{
    for (vx_int32 i = 0; i < num_dims; i++)
    {
        dimensions[i] = dims[i];
    }
}

static vx_status VX_CALLBACK vxNNEFInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_char perror[MAXLEN] = "";

    vx_status status = VX_SUCCESS;

    // copy NNEF graph to node
    node->attributes.localDataPtr = nnef_graph_copy(node->kernel->attributes.localDataPtr);
   
    if (!nnef_graph_allocate_buffers(node->attributes.localDataPtr, perror))
    {
        //nnef allocate buffers failed
        printf("[nnef_graph_allocate_buffers] error:%s\n", perror);
        status = VX_FAILURE;
    }

    return status;
}

static vx_status VX_CALLBACK vxNNEFDeinitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0;

    vx_meta_format *meta = node->kernel->signature.meta_formats;;

    for (i = 0; i < num; i++)
    {
        ownReleaseMetaFormat(&meta[i]);
    }
    // destroy NNEF graph in node 
    nnef_graph_release(node->attributes.localDataPtr);
    
    node->attributes.localDataPtr = NULL;
 
    return status;
}

static vx_status VX_CALLBACK vxNNEFKernelDeinitializer(vx_kernel nn_kernel)
{
    vx_status status = VX_SUCCESS;

    // destroy NNEF graph in kernel 
    nnef_graph_release(nn_kernel->attributes.localDataPtr);

    nn_kernel->attributes.localDataPtr = NULL;

    // Remove the kernel
    vxRemoveKernel(nn_kernel);

    return status;
}

static vx_status VX_CALLBACK vxNNEFValidator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format meta[])
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0;

    for (i = 0; i < num; i++)
    {
        status = vxSetMetaFormatFromReference(meta[i], parameters[i]);
    }

    return status;
}

static vx_status VX_CALLBACK vxNNEFKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_uint32 i = 0, j = 0, input_num = 0, output_num = 0;
    vx_char perror[MAXLEN] = "";
    vx_status status = VX_SUCCESS;
    const vx_char** inputs = NULL, **outputs = NULL;

    // Get NNEF graph from kernel attributes
    nnef_graph_t nnef_graph_node = node->attributes.localDataPtr;

    input_num = nnef_graph_input_names(nnef_graph_node, NULL);
    output_num = nnef_graph_output_names(nnef_graph_node, NULL);

    // alloc memory according to input and output number
    inputs = (const vx_char **)malloc(sizeof(const vx_char *) * input_num);
    outputs = (const vx_char **)malloc(sizeof(const vx_char *) * output_num);

    // get inputs and outputs
    nnef_graph_input_names(nnef_graph_node, inputs);
    nnef_graph_output_names(nnef_graph_node, outputs);

    vx_tensor *tensors = (vx_tensor *)malloc(sizeof(vx_tensor) * num);

    if (tensors == NULL)
    {
        status = VX_ERROR_NO_MEMORY;
        return status;
    }

    // get input vx_tensors and set vx_tensors into NNEF graph tensor
    for (i = 0; i < input_num; i++)
    {
        tensors[i] = (vx_tensor)parameters[i];

        size_t input_tensor_data_size = compute_patch_size(tensors[i]->dimensions, tensors[i]->number_of_dimensions) * 
                                                           sizeof_tensor_type(tensors[i]->data_type);
        
        memcpy(nnef_tensor_data(nnef_graph_find_tensor(nnef_graph_node, inputs[i])), tensors[i]->addr, input_tensor_data_size);
    }

    //Execute nnef kernel
    if (!nnef_graph_execute(nnef_graph_node, perror))
    {
        printf("[nnef_graph_execute] error:%s\n", perror);
        status = VX_FAILURE;
    }

    // get output vx_tensors
    for (i = input_num, j = 0; i < num; i++, j++)
    {
        tensors[i] = (vx_tensor)parameters[i];

        size_t output_tensor_data_size = compute_patch_size(tensors[i]->dimensions, tensors[i]->number_of_dimensions) * 
                                                            sizeof_tensor_type(tensors[i]->data_type);

        tensors[i]->addr = malloc(output_tensor_data_size);

        memcpy(tensors[i]->addr, nnef_tensor_data(nnef_graph_find_tensor(nnef_graph_node, outputs[j])), output_tensor_data_size);
    }

    if (NULL != tensors)
        free(tensors);
    if (NULL != inputs)
        free(inputs);
    if (NULL != outputs)
        free(outputs);
    
    return status;
}

 /*! \brief The Entry point into a user defined kernel module */
static vx_kernel CreateNNEFKernel(vx_context context, vx_int32 input_num, vx_int32 output_num, const vx_char * kernel_name)
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0;
    vx_kernel kernel;

    vx_int32 num_params = input_num + output_num;
    vx_param_description_t *nnef_kernel_params = (vx_param_description_t *)malloc(num_params * sizeof(vx_param_description_t));

    // input tensor type
    for (i = 0; i < input_num; i++)
    {
        nnef_kernel_params[i].direction = VX_INPUT;
        nnef_kernel_params[i].data_type = VX_TYPE_TENSOR;
        nnef_kernel_params[i].state = VX_PARAMETER_STATE_REQUIRED;
    }

    // output tensor type
    for (i = input_num; i < num_params; i++)
    {
        nnef_kernel_params[i].direction = VX_OUTPUT;
        nnef_kernel_params[i].data_type = VX_TYPE_TENSOR;
        nnef_kernel_params[i].state = VX_PARAMETER_STATE_REQUIRED;
    }

    static vx_int32 kernel_enum = VX_KERNEL_BASE(VX_ID_USER, VX_LIBRARY_KHR_BASE);

    // set NNEF kernel description
    kernel = vxAddUserKernel(context, kernel_name, kernel_enum++, vxNNEFKernel,
                                num_params, vxNNEFValidator, vxNNEFInitializer, vxNNEFDeinitializer);
    kernel->kernel_object_deinitialize = vxNNEFKernelDeinitializer;

    if (kernel)
    {
        vx_uint32 p = 0;
        for (p = 0; p < num_params; p++)
        {
            status = vxAddParameterToKernel(kernel, p,
                                            nnef_kernel_params[p].direction,
                                            nnef_kernel_params[p].data_type,
                                            nnef_kernel_params[p].state);
        }
        if (status != VX_SUCCESS)
        {
            vxRemoveKernel(kernel);
        }
        else
        {
            status = vxFinalizeKernel(kernel);
        }
        if (status != VX_SUCCESS)
        {
            printf("Failed to publish kernel %s\n", kernel_name);
        }
    }
    if (NULL != nnef_kernel_params)
        free(nnef_kernel_params);

    return kernel;
}

static vx_status vxSetMetaFormatNNEF(void *nnef_graph, vx_meta_format meta, const vx_char * tensor_name)
{
    vx_status status = VX_SUCCESS;
    vx_int32 i = 0;

    vx_uint8 fixed_point_pos_out = 0;
    const vx_char *tensor_type_name = NULL;
    vx_size num_dims = 0;
    const vx_int32 *dims = NULL;
    vx_int32 type = 0;

    // find tensor by name
    nnef_tensor_t nnef_tensor = nnef_graph_find_tensor(nnef_graph, tensor_name);
    // query NNEF tensor number of dim
    num_dims = nnef_tensor_rank(nnef_tensor);

    // query NNEF tensor dims
    dims = nnef_tensor_dims(nnef_tensor);

    // copying the dims into dimensions since OpenVX tensor created requires size_t
    vx_size *dimensions = (vx_size *)malloc(num_dims * sizeof(vx_size));
    copy_dims(dims, num_dims, dimensions);
    // query NNEF tensor name
    tensor_type_name = nnef_tensor_dtype(nnef_tensor);
    // convert NNEF tensor name into OpenVX tensor type
    type = tensor_vx_type(tensor_type_name);

    status = vxSetMetaFormatAttribute(meta, VX_TENSOR_DATA_TYPE, &type, sizeof(vx_enum));
    status = vxSetMetaFormatAttribute(meta, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos_out, sizeof(fixed_point_pos_out));
    status = vxSetMetaFormatAttribute(meta, VX_TENSOR_DIMS, dimensions, sizeof(vx_size) * num_dims);
    status = vxSetMetaFormatAttribute(meta, VX_TENSOR_NUMBER_OF_DIMS, &num_dims, sizeof(size_t));

    if (NULL != dimensions)
        free(dimensions);

    return status;
}

VX_API_ENTRY vx_kernel VX_API_CALL vxImportKernelFromURL(vx_context context, const vx_char * type, const vx_char * url)
{
    vx_kernel kernel = NULL;
    vx_int32 i = 0, j = 0;

    vx_char perror[MAXLEN] = "";
    vx_char kernel_name[MAXLEN] = "";
    static vx_int32 counter = 1;

    vx_int32 input_num = 0, output_num = 0, num = 0;
    const vx_char** inputs = NULL, **outputs = NULL;

    nnef_graph_t nnef_graph = nnef_graph_load(url, perror);

    if (!nnef_graph)
    {
        //load nnef graph failed
        printf("Failed to load nnef graph %s\n", perror);
        return NULL;
    }

    if (!nnef_graph_infer_shapes(nnef_graph, perror))
    {
        //infer shapes failed
        printf("[nnef_graph_infer_shapes] error:%s\n", perror);
        return NULL;
    }

    // get input and output number first
    input_num = nnef_graph_input_names(nnef_graph, NULL);
    output_num = nnef_graph_output_names(nnef_graph, NULL);
    num = input_num + output_num;

    // alloc memory according to input and output number
    inputs = (const vx_char **)malloc(sizeof(const vx_char *) * input_num);
    outputs = (const vx_char **)malloc(sizeof(const vx_char *) * output_num);

    // get inputs and outputs
    nnef_graph_input_names(nnef_graph, inputs);
    nnef_graph_output_names(nnef_graph, outputs);

    sprintf(kernel_name, "nnef.import.%d", counter++);

    kernel = CreateNNEFKernel(context, input_num, output_num, kernel_name);
    
    kernel->attributes.localDataPtr = nnef_graph;

    vx_meta_format *meta;

    meta = kernel->signature.meta_formats;

    for (i = 0; i < num; i++)
    {
        meta[i] = ownCreateMetaFormat(context);
        meta[i]->type = VX_TYPE_TENSOR;
    }

    for (i = 0; i < input_num; i++)
        vxSetMetaFormatNNEF(nnef_graph, meta[i], inputs[i]);
    for (i = input_num, j = 0; i < num; i++, j++)
        vxSetMetaFormatNNEF(nnef_graph, meta[i], outputs[j]);

    if (NULL != inputs)
        free(inputs);
    if (NULL != outputs)
        free(outputs);

    return kernel;
}

#endif