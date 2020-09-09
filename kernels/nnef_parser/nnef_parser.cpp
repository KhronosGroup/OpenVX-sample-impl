/*
 * Copyright (c) 2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstring>
#include "nnef_parser.h"
#include "nnef.h"

using namespace nnef;

nnef_tensor_t nnef_new_tensor(void)
{

    Tensor *tensor = new Tensor();
    return tensor;
}

void nnef_free_tensor(nnef_tensor_t tensor)
{
    Tensor *nnef_tensor = (Tensor *)tensor;
    if (nnef_tensor != NULL)
    {
        delete nnef_tensor;
    }
}

nnef_graph_t nnef_load_graph(const char * url, char *perror)
{
    std::string path = url;
    std::string error;
    Graph *nnef_graph = new Graph();
    int flag = load_graph(path, *nnef_graph, error);
    if (!flag)
    {
        if (perror != NULL)
        {
            strncpy(perror, error.c_str(), error.length() + 1);
        }

        return NULL;
    }
    return nnef_graph;
}

nnef_graph_t nnef_graph_copy(nnef_graph_t pgraph)
{
    Graph *nnef_graph = (Graph *)pgraph;
    return new Graph(*nnef_graph);
}

int nnef_infer_shapes(nnef_graph_t pgraph, char *perror)
{
    std::string error;

    Graph *nnef_graph = (Graph *)pgraph;

    if (!infer_shapes(*nnef_graph, error))
    {
        if (perror != NULL)
        {
            strncpy(perror, error.c_str(), error.length() + 1);
        }
        return 0;
    }
    return 1;
}

int nnef_allocate_buffers(nnef_graph_t pgraph, char *perror)
{
    std::string error;
    Graph *nnef_graph = (Graph *)pgraph;

    if (nnef_graph == NULL) return 0;

    if (!nnef::allocate_buffers(*nnef_graph, error))
    {
        if (perror != NULL)
        {
            strncpy(perror, error.c_str(), error.length() + 1);
        }
        return 0;
    }
    return 1;
}

size_t nnef_get_tensor_rank(nnef_tensor_t tensor)
{
    Tensor *nnef_tensor = (Tensor *)tensor;

    return nnef_tensor->shape.size();
}

char * nnef_get_tensor_dtype(nnef_tensor_t tensor)
{
    Tensor *nnef_tensor = (Tensor *)tensor;

    return const_cast<char*>(nnef_tensor->dtype.c_str());
}

void nnef_get_tensor_dims(nnef_tensor_t tensor, size_t *dims)
{
    Tensor *nnef_tensor = (Tensor *)tensor;

    int nnef_num_dim = nnef_get_tensor_rank(tensor);

    int *tem_dim = &nnef_tensor->shape[0];
    for (int i = 0; i < nnef_num_dim; i++)
    {
        dims[i] = tem_dim[i];
    }
}

int nnef_graph_inputs(nnef_graph_t pgraph, char** inputs)
{
    Graph *nnef_graph = (Graph *)pgraph;

    if (inputs != NULL)
    {
        for (int i = 0; i < nnef_graph->inputs.size(); i++)
        {
            inputs[i] = const_cast<char*>(nnef_graph->inputs[i].c_str());
        }
    }
    return nnef_graph->inputs.size();
}

int nnef_graph_outputs(nnef_graph_t pgraph, char** outputs)
{
    Graph *nnef_graph = (Graph *)pgraph;

    if (outputs != NULL)
    {
        for (int i = 0; i < nnef_graph->outputs.size(); i++)
        {
            outputs[i] = const_cast<char*>(nnef_graph->outputs[i].c_str());
        }
    }
    return nnef_graph->outputs.size();
}

int nnef_read_tensor(const char * url, nnef_tensor_t tensor, char *perror)
{
    std::string error;

    Tensor *nnef_tensor = (Tensor *)tensor;

    if (nnef_tensor == NULL) return 1;

    const std::string& filename = url;
    if (!read_tensor(filename, *nnef_tensor, error))
    {
        if (perror != NULL)
        {
            strncpy(perror, error.c_str(), error.length() + 1);
        }
        return 1;
    }
    return 0;
}

int nnef_write_tensor(const char * url, nnef_tensor_t tensor, char *perror)
{
    std::string error;

    Tensor *nnef_tensor = (Tensor *)tensor;

    if (nnef_tensor == NULL) return 1;

    const std::string& filename = url;
    if (!write_tensor(filename, *nnef_tensor, error))
    {
        if (perror != NULL)
        {
            strncpy(perror, error.c_str(), error.length() + 1);
        }
        return 1;
    }
    return 0;
}

nnef_tensor_t nnef_graph_find_tensor(nnef_graph_t pgraph, char* tensor_name)
{
    Graph *nnef_graph = (Graph *)pgraph;

    if (nnef_graph == NULL) return NULL;

    Tensor * tensor = &nnef_graph->tensors.at(tensor_name);

    return tensor;
}

void * nnef_get_tensor_data(nnef_tensor_t tensor)
{
    Tensor *nnef_tensor = (Tensor *)tensor;

    return &nnef_tensor->data[0];
}

int nnef_execute(nnef_graph_t pgraph, char *perror)
{
    std::string error;
    Graph *nnef_graph = (Graph *)pgraph;

    if (nnef_graph == NULL)
        return 0;

    if (!nnef::execute(*nnef_graph, error))
    {
        if (perror != NULL)
        {
            strncpy(perror, error.c_str(), error.length() + 1);
        }
        return 0;
    }
    return 1;
}

void nnef_destroy_graph(nnef_graph_t pgraph)
{
    Graph *nnef_graph = (Graph *)pgraph;
    
    if (nnef_graph)
    {
        delete nnef_graph;
        nnef_graph = NULL;
    }
}
