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

#ifndef _NNEF_PARSER_H_
#define _NNEF_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef    __cplusplus
#if _WIN32
#define EXPORTDLL extern "C" __declspec(dllexport)
#else
#define EXPORTDLL extern "C"
#endif
#else  // __cplusplus
#if _WIN32
#define EXPORTDLL __declspec(dllexport)
#else
#define EXPORTDLL
#endif
#endif // __cplusplus

    typedef void * nnef_graph_t;

    typedef void * nnef_tensor_t;

    /*
     * Create a new tensor
     *
     * @return tensor
     */
    EXPORTDLL
        nnef_tensor_t nnef_new_tensor(void);

    /*
     * free the tensor
     */
    EXPORTDLL
        void nnef_free_tensor(nnef_tensor_t tensor);

    /*
     * Load NNEF graph from url
     *
     * @param url: the path to the top level NNEF model folder
     * @param perror: the string to store the error message if any
     *
     * @return NNEF graph
     */
    EXPORTDLL
        nnef_graph_t nnef_load_graph(const char * url, char *perror);

    /*
     * copy one NNEF graph
     *
     * @param pgraph: NNEF graph
     *
     * @return the copy of NNEF graph
     */
    EXPORTDLL
        nnef_graph_t nnef_graph_copy(nnef_graph_t pgraph);

    /*
     * Perform shape inference on the graph
     *
     * @param pgraph: the graph object
     * @param perror: the string to store the error message if any
     *
     * @return true if there were no errors, false otherwise
     */
    EXPORTDLL
        int nnef_infer_shapes(nnef_graph_t pgraph, char *perror);

    /*
     * Allocate tensor buffers in the graph
     *
     * @param pgraph: the graph object
     * @param perror: the string to store the error message if any
     *
     * @return true if there were no errors, false otherwise
     */
    EXPORTDLL
        int nnef_allocate_buffers(nnef_graph_t pgraph, char *perror);

    /*
     * query tensor rank from tensor
     *
     * @param tensor: tensor
     *
     * @return rank
     */
    EXPORTDLL
        size_t nnef_get_tensor_rank(nnef_tensor_t tensor);

    /*
     * query tensor type from tensor
     *
     * @param tensor: tensor
     *
     * @return type
     */
    EXPORTDLL
        char *nnef_get_tensor_dtype(nnef_tensor_t tensor);

    /*
     * query tensor dims from tensor
     *
     * @param tensor: tensor
     *
     */
    EXPORTDLL
        void nnef_get_tensor_dims(nnef_tensor_t tensor, size_t *dims);

    /*
     * query input number and input names from NNEF graph
     *
     * @param pgraph: NNEF graph
     * @param inputs: input names
     *
     * @return input number
     */
    EXPORTDLL
        int nnef_graph_inputs(nnef_graph_t pgraph, char** inputs);

    /*
     * query output number and output names from NNEF graph
     *
     * @param pgraph: NNEF graph
     * @param inputs: output names
     *
     * @return output number
     */
    EXPORTDLL
        int nnef_graph_outputs(nnef_graph_t pgraph, char** outputs);

    /*
     * Read tensor from binary file
     *
     * @param url: the name of the file to read from
     * @param tensor: tensor
     * @param perror: the string to store the error message if any
     *
     * @return true if there were no errors, false otherwise
     */
    EXPORTDLL
        int nnef_read_tensor(const char * url, nnef_tensor_t tensor, char *perror);

    /*
     * Write tensor to binary file
     *
     * @param url: the name of the file to write to
     * @param tensor: tensor
     * @param perror: the string to store the error message if any
     *
     * @return true if there were no errors, false otherwise
     */
    EXPORTDLL
        int nnef_write_tensor(const char * url, nnef_tensor_t tensor, char *perror);

    /*
     * Find tensor from NNEF graph by name
     *
     * @param pgraph: NNEF graph
     * @param tensor_name: tensor name
     *
     * @return tensor
     */
    EXPORTDLL
        nnef_tensor_t nnef_graph_find_tensor(nnef_graph_t pgraph, char* tensor_name);

    /*
     * Get tensor data from tensor
     *
     * @param tensor: tensor
     *
     * @return tensor data
     */
    EXPORTDLL
        void * nnef_get_tensor_data(nnef_tensor_t tensor);

    /*
     * Execute a graph
     *
     * @param pgraph: the graph object
     * @param perror: the string to store the error message if any
     *
     * @return true if there were no errors, false otherwise
     */
    EXPORTDLL
        int nnef_execute(nnef_graph_t pgraph, char *perror);

    /*
     * destroy NNEF graph
     *
     * @param pgraph: NNEF graph
     */
    EXPORTDLL
        void nnef_destroy_graph(nnef_graph_t pgraph);

#ifdef __cplusplus
}
#endif

#endif
