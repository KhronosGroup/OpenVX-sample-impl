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

/*!
 * \file
 * \brief The OpenVX implementation unit test code.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vxu.h>
/* TODO: remove vx_compatibility.h after transition period */
#include <VX/vx_compatibility.h>

#include <VX/vx_lib_debug.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_lib_xyz.h>

#if defined(EXPERIMENTAL_USE_DOT)
#include <VX/vx_khr_dot.h>
#endif

#if defined(OPENVX_USE_XML)
#include <VX/vx_khr_xml.h>
#endif

#include <VX/vx_helper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>

#define VX_KERNEL_FAKE_MAX  (VX_KERNEL_CHANNEL_EXTRACT) // supposed to be VX_KERNEL_MAX but until all the kernels are implemented, this will be used.

/*
 * We unfortunately have to use different macros at the call-level for
 * empty and non-empty argument lists, as C99 syntax mandates presence
 * of the first argument for variable-argument lists (as opposed to e.g.
 * the GNU C extension). This is worked around in the shared
 * implementation by "pasting" an empty string argument.
 */
#define VALARM(message, ...)  printf("[%s:%u] " message "\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#define ALARM(message) VALARM("%s", message)
#define VFAIL(label, message, ...) do { \
    VALARM(message, __VA_ARGS__); \
    if (status == VX_SUCCESS) \
        status = VX_FAILURE; \
    goto label; \
 } while (0)
#define FAIL(label, message) VFAIL(label, message "%s", "")

#define CHECK_ALL_ITEMS(array, iter, status, label) { \
    status = VX_SUCCESS; \
    for ((iter) = 0; (iter) < dimof(array); (iter)++) { \
        if ((array)[(iter)] == 0) { \
            printf("Item %u in "#array" is null!\n", (iter)); \
            assert((array)[(iter)] != 0); \
            status = VX_ERROR_NOT_SUFFICIENT; \
        } \
    } \
    if (status != VX_SUCCESS) { \
        goto label; \
    } \
}

/*! \brief A local definition to point to a specific unit test */
typedef vx_status (*vx_unittest_f)(int argc, char *argv[]);

/*! \brief The structure which correlates each unit test with a result and a name. */
typedef struct _vx_unittest_t {
    vx_status status;
    vx_char name[VX_MAX_KERNEL_NAME];
    vx_unittest_f unittest;
} vx_unittest;

void vx_print_addressing(void *ptr, vx_imagepatch_addressing_t *addr)
{
    printf("ptr=%p dim={%u,%u} stride={%d,%d} scale={%u,%u} step={%u,%u}\n",
            ptr,
            addr->dim_x, addr->dim_y,
            addr->stride_x, addr->stride_y,
            addr->scale_x, addr->scale_y,
            addr->step_x, addr->step_y);
}

static void vx_print_log(vx_reference ref)
{
    char message[VX_MAX_LOG_MESSAGE_LEN];
    vx_uint32 errnum = 1;
    vx_status status = VX_SUCCESS;
    do {
        status = vxGetLogEntry(ref, message);
        if (status != VX_SUCCESS)
            printf("[%05u] error=%d %s", errnum++, status, message);
    } while (status != VX_SUCCESS);
}

#define NUM_BUFS (2)
#define NUM_IMGS (2)
#define BUF_SIZE (1024)
#undef PLANES
#define PLANES (4)
#define HEIGHT (480)
#define WIDTH (640)
#define CHANNELS (1)

uint8_t images[NUM_IMGS][PLANES][HEIGHT][WIDTH][CHANNELS];

//**********************************************************************
//
//**********************************************************************

/*!
 * \brief Test creating and releasing a kernel in a node.
 * \ingroup group_tests
 */
vx_status vx_test_framework_load_extension(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        status = vxLoadKernels(context, "xyz");
        if (status == VX_SUCCESS)
        {
            vx_kernel kernel = vxGetKernelByName(context, "org.khronos.example.xyz");
            if (kernel)
            {
                vx_uint32 num_kernels = 0;
                vx_uint32 num_modules = 0;
                vx_uint32 num_references = 0;
                status = VX_SUCCESS;
                if (vxQueryContext(context, VX_CONTEXT_UNIQUE_KERNELS, &num_kernels, sizeof(vx_uint32)) != VX_SUCCESS)
                    status = VX_ERROR_NOT_SUFFICIENT;
                if (vxQueryContext(context, VX_CONTEXT_MODULES, &num_modules, sizeof(vx_uint32)) != VX_SUCCESS)
                    status = VX_ERROR_NOT_SUFFICIENT;
                if (vxQueryContext(context, VX_CONTEXT_REFERENCES, &num_references, sizeof(vx_uint32)) != VX_SUCCESS)
                    status = VX_ERROR_NOT_SUFFICIENT;
                printf("[VX_TEST] Kernels:%u Modules:%u Refs:%u\n", num_kernels, num_modules, num_references);
                vxReleaseKernel(&kernel);
                status = VX_SUCCESS;
            }
            status = vxUnloadKernels(context, "xyz");
        }
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Loaded Kernel Node Creation Test
 */
vx_status vx_test_framework_load_kernel_node(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        if (vxLoadKernels(context, "xyz") == VX_SUCCESS)
        {
            vx_kernel kernel = vxGetKernelByName(context, "org.khronos.example.xyz");
            if (kernel)
            {
                vx_graph graph = vxCreateGraph(context);
                if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
                {
                    vx_image input = vxCreateImage(context, 640, 480, VX_DF_IMAGE_U8);
                    vx_image output = vxCreateImage(context, 640, 480, VX_DF_IMAGE_U8);
                    vx_array temp = vxCreateArray(context, VX_TYPE_UINT8, 10);
                    // we're also testing the "set parameter inside the wrapper. It will check for 0 refs.
                    vx_node xyz = vxXYZNode(graph, input, 10, output, temp);
                    if (xyz)
                    {
                        vx_size size = 0;
                        void *ptr = NULL;
                        status = vxQueryNode(xyz, VX_NODE_LOCAL_DATA_SIZE, &size, sizeof(size));
                        status = vxQueryNode(xyz, VX_NODE_LOCAL_DATA_PTR, &ptr, sizeof(ptr));
                        if (status == VX_SUCCESS && size == XYZ_DATA_AREA && ptr != NULL)
                        {
                            printf("Node private size="VX_FMT_SIZE" ptr=%p\n",size,ptr);
                            status = VX_SUCCESS;
                        }
                        vxReleaseNode(&xyz);
                    }
                    vx_print_log((vx_reference)graph);
                    vxReleaseGraph(&graph);
                }
                vxReleaseKernel(&kernel);
            }
            status = vxUnloadKernels(context, "xyz");
        }
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Test creating and releasing a kernel in a node.
 * \ingroup group_tests
 */
vx_status vx_test_framework_copy(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_uint32 width = 640;
            vx_uint32 height = 480;
            vx_image input = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_image output = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_node node = 0;
            vx_uint32 errors = 0u;

            if (input == 0 || output == 0) {
                ALARM("failed to create images");
                return VX_ERROR_NOT_SUPPORTED;
            }
            status |= vxLoadKernels(context, "openvx-debug");
            node = vxCopyImageNode(graph, input, output);
            if (node)
            {
                vx_uint32 numNodes = 0;
                if (vxQueryGraph(graph, VX_GRAPH_NUMNODES, &numNodes, sizeof(numNodes)) != VX_SUCCESS)
                {
                    status = VX_ERROR_NOT_SUFFICIENT;
                }
                else
                {
                    if (numNodes != 1)
                    {
                        status = VX_ERROR_NOT_SUFFICIENT;
                    }
                    else
                    {
                        status = vxVerifyGraph(graph);
                        if (status == VX_SUCCESS)
                        {
                            status |= vxuFillImage(context, 0x42, input);

                            // actually run the graph
                            status |= vxProcessGraph(graph);
                            if (status != VX_SUCCESS)
                            {
                                ALARM("Failed to Process Graph!");
                            }
                            else
                            {
                                status = vxuCheckImage(context, output, 0x42, &errors);
                                if (status != VX_SUCCESS || errors > 0)
                                {
                                    ALARM("Image Data was not copied");
                                }
                            }
                        }
                        else
                        {
                            ALARM("Failed Graph Verification");
                        }
                    }
                }
                vxReleaseNode(&node);
            }
            else
            {
                ALARM("failed to create node");
                status = VX_ERROR_NOT_SUFFICIENT;
                goto exit;
            }
            vx_print_log((vx_reference)graph);
            vxReleaseGraph(&graph);
            status |= vxUnloadKernels(context, "openvx-debug");
        }
        else
        {
            ALARM("failed to create graph");
            status = VX_ERROR_NOT_SUFFICIENT;
            goto exit;
        }
exit:
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Test creating and releasing a copy node with virtual object output.
 * \ingroup group_tests
 */
vx_status vx_test_framework_copy_virtual(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 width = 640;
        vx_uint32 height = 480;
        vx_uint32 n;
        vx_image images[] = {
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),
        };
        vx_graph graph = vxCreateGraph(context);
        status |= vxLoadKernels(context, "openvx-debug");
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_uint32 errors = 0u;
            vx_image virt = vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT);
            vx_node nodes[] = {
                vxCopyImageNode(graph, images[0], virt),
                vxCopyImageNode(graph, virt, images[1]),
            };
            vx_uint32 numNodes = 0;
            if (vxQueryGraph(graph, VX_GRAPH_NUMNODES, &numNodes, sizeof(numNodes)) != VX_SUCCESS)
            {
                status = VX_ERROR_NOT_SUFFICIENT;
            }
            else
            {
                if (numNodes != 2)
                {
                    status = VX_ERROR_NOT_SUFFICIENT;
                }
                else
                {
                    status = vxVerifyGraph(graph);
                    if (status == VX_SUCCESS)
                    {
                        status |= vxuFillImage(context, 0x42, images[0]);

                        // actually run the graph
                        status |= vxProcessGraph(graph);
                        if (status != VX_SUCCESS)
                        {
                            ALARM("Failed to Process Graph!");
                        }
                        else
                        {
                            status = vxuCheckImage(context, images[1], 0x42, &errors);
                            if (status != VX_SUCCESS || errors > 0)
                            {
                                ALARM("Image Data was not copied");
                                assert(errors == 0);
                            }
                        }
                    }
                    else
                    {
                        ALARM("Failed Graph Verification");
                    }
                }
            }
            for (n = 0; n < dimof(nodes); n++) {
                vxReleaseNode(&nodes[n]);
            }
            vxReleaseImage(&virt);
            vxReleaseGraph(&graph);
        }
        else
        {
            ALARM("failed to create graph");
            status = VX_ERROR_NOT_SUFFICIENT;
            goto exit;
        }
        for (n = 0; n < dimof(images); n++) {
            vxReleaseImage(&images[n]);
        }
        status |= vxUnloadKernels(context, "openvx-debug");
exit:
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Test creating 2 nodes with a virtual image between inputs
 * and outputs.
 * \ingroup group_tests
 */
vx_status vx_test_framework_virtualimage(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_uint32 width = 640u;
            vx_uint32 height = 480u;
            vx_uint32 errors = 0u;
            vx_image input = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_image virt = vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT);
            vx_image output = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_node nodes[2];
            vx_imagepatch_addressing_t addr;
            void *base = NULL;
            vx_uint8 pixels[2];

            vx_rectangle_t rect = {0, 0, width, height};

            if (input == 0 || output == 0 || virt == 0) {
                ALARM("failed to create images");
                return VX_ERROR_NOT_SUPPORTED;
            }

            status = vxAccessImagePatch(virt, &rect, 0, &addr, &base, VX_READ_AND_WRITE);
            if (status != VX_ERROR_OPTIMIZED_AWAY) {
                ALARM("failed to prevent access to virtual image!");
                return VX_ERROR_NOT_SUFFICIENT;
            }
            // declare a non-zero rectangle
            rect.end_x = 2;
            rect.end_y = 1;
            status = vxCommitImagePatch(virt, &rect, 0, &addr, pixels);
            if (status != VX_ERROR_OPTIMIZED_AWAY) {
                ALARM("failed to prevent commit to virtual image!");
                return VX_ERROR_NOT_SUFFICIENT;
            }

            status |= vxLoadKernels(context, "openvx-debug");
            nodes[0] = vxCopyImageNode(graph, input, virt);
            nodes[1] = vxCopyImageNode(graph, virt, output);

            if (nodes[0] && nodes[1])
            {
                status = vxVerifyGraph(graph);
                if (status == VX_SUCCESS)
                {
                    vxuFillImage(context, 0x42, input);

                    status = vxProcessGraph(graph);
                    if (status != VX_SUCCESS)
                    {
                        ALARM("Failed to Process Graph!");
                    }
                    else
                    {
                        status = vxuCheckImage(context, output, 0x42, &errors);
                        if (status != VX_SUCCESS)
                        {
                            ALARM("Image Data was not copied");
                        }
                    }
                }
                else
                {
                    ALARM("Failed Graph Verification");
                }
                vxReleaseNode(&nodes[0]);
                vxReleaseNode(&nodes[1]);
            }
            else
            {
                ALARM("failed to create node");
                status = VX_ERROR_NOT_SUFFICIENT;
                goto exit;
            }
            vx_print_log((vx_reference)graph);
            vxReleaseGraph(&graph);

            status |= vxUnloadKernels(context, "openvx-debug");
        }
        else
        {
            ALARM("failed to create graph");
            status = VX_ERROR_NOT_SUFFICIENT;
            goto exit;
        }
exit:
        vxReleaseContext(&context);
    }
    return status;
}


/*!
 * \brief Test creating and releasing a kernel in a node.
 * \ingroup group_tests
 */
vx_status vx_test_framework_heads(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_uint32 width = 640;
            vx_uint32 height = 480;
            vx_image input = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_image output = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_node nodes[2];

            if (input == 0 || output == 0)
            {
                ALARM("failed to create images");
                return VX_ERROR_NOT_SUPPORTED;
            }
            status |= vxLoadKernels(context, "openvx-debug");
            nodes[0] = vxCopyImageNode(graph, input, output);
            nodes[1] = vxCopyImageNode(graph, output, input);
            if (nodes[0] && nodes[1])
            {
                status = vxVerifyGraph(graph);
                if (status == VX_ERROR_INVALID_GRAPH)
                {
                    // we're trying to detect an error case.
                    status = VX_SUCCESS;
                }
                else
                {
                    ALARM("Failed to detect cycle in graph!");
                    status = VX_ERROR_NOT_SUFFICIENT;
                }
                vxReleaseNode(&nodes[0]);
                vxReleaseNode(&nodes[1]);
            }
            else
            {
                ALARM("failed to create node");
                status = VX_ERROR_NOT_SUFFICIENT;
                goto exit;
            }
            vx_print_log((vx_reference)graph);
            vxReleaseGraph(&graph);

            status |= vxUnloadKernels(context, "openvx-debug");
        }
        else
        {
            ALARM("failed to create graph");
            status = VX_ERROR_NOT_SUFFICIENT;
            goto exit;
        }
exit:
        vxReleaseContext(&context);
    }
    return status;
}


/*!
 * \brief Test creating and releasing a kernel in a node.
 * \ingroup group_tests
 */
vx_status vx_test_framework_unvisited(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_uint32 width = 640;
            vx_uint32 height = 480;
            vx_array arr_in = vxCreateArray(context, VX_TYPE_UINT32, 10);
            vx_array arr_out = vxCreateArray(context, VX_TYPE_UINT32, 10);
            vx_image input = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_image output = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
            vx_node nodes[3];

            if (input == 0 || output == 0)
            {
                ALARM("failed to create images");
                return VX_ERROR_NOT_SUPPORTED;
            }
            status |= vxLoadKernels(context, "openvx-debug");
            nodes[0] = vxCopyImageNode(graph, input, output);
            nodes[1] = vxCopyImageNode(graph, output, input);
            nodes[2] = vxCopyArrayNode(graph, arr_in, arr_out);
            if (nodes[0] && nodes[1] && nodes[2])
            {
                status = vxVerifyGraph(graph);
                if (status == VX_ERROR_INVALID_GRAPH)
                {
                    // we're trying to detect an error case.
                    status = VX_SUCCESS;
                }
                else
                {
                    ALARM("Failed to detect cycle in graph!");
                    status = VX_ERROR_NOT_SUFFICIENT;
                }
                vxReleaseNode(&nodes[0]);
                vxReleaseNode(&nodes[1]);
                vxReleaseNode(&nodes[2]);
            }
            else
            {
                ALARM("failed to create node");
                status = VX_ERROR_NOT_SUFFICIENT;
                goto exit;
            }
            vx_print_log((vx_reference)graph);
            vxReleaseGraph(&graph);

            status |= vxUnloadKernels(context, "openvx-debug");
        }
        else
        {
            ALARM("failed to create graph");
            status = VX_ERROR_NOT_SUFFICIENT;
            goto exit;
        }
exit:
        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_test_framework_kernels(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        char kernelName[] = "org.khronos.openvx.box_3x3";
        vx_kernel boxdup = vxGetKernelByName(context, kernelName);
        status = vxGetStatus((vx_reference)boxdup);
        if (status != VX_SUCCESS)
        {
            status = VX_ERROR_NOT_SUFFICIENT;
            goto exit;
        }
        else
        {
            vx_graph graph = vxCreateGraph(context);
            vx_kernel kernel = vxGetKernelByEnum(context, VX_KERNEL_BOX_3x3);
            vx_node node = vxCreateGenericNode(graph, kernel);
            status = vxGetStatus((vx_reference)node);
            if (status != VX_SUCCESS)
                goto exit2;
exit2:
            vxReleaseKernel(&kernel);
            vxReleaseNode(&node);
            vxReleaseGraph(&graph);
        }
exit:
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Tests delay object creation.
 * \ingroup group_tests
 */
vx_status vx_test_framework_delay_graph(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_uint32 w = 640, h = 480;
    vx_uint32 errors = 0u;
    vx_df_image f = VX_DF_IMAGE_U8;
    vx_uint32 i;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
#define DEPTH_TEST (3)
            vx_image images[DEPTH_TEST];
            vx_node nodes[DEPTH_TEST];
            vx_delay delay = 0;

            memset(images, 0, sizeof(images));
            memset(nodes, 0, sizeof(nodes));

            for (i = 0; i < DEPTH_TEST; i++)
            {
                images[i] = vxCreateImage(context, w, h, f);
                if (images[i] == 0)
                {
                    status = VX_ERROR_NO_MEMORY;
                    goto exit;
                }
            }
            delay = vxCreateDelay(context, (vx_reference)images[0], DEPTH_TEST-1);
            status |= vxLoadKernels(context, "openvx-debug");
            nodes[0] = vxCopyImageNode(graph, images[0], (vx_image)vxGetReferenceFromDelay(delay, 0));
            nodes[1] = vxCopyImageNode(graph, (vx_image)vxGetReferenceFromDelay(delay, -1), images[1]);
            nodes[2] = vxCopyImageNode(graph, (vx_image)vxGetReferenceFromDelay(delay, -1), images[2]);

            for (i = 0; i < dimof(nodes); i++)
            {
                if (nodes[i] == 0)
                {
                    status = VX_ERROR_NOT_SUFFICIENT;
                    goto exit;
                }
            }
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                vxuFillImage(context, 0xBE, images[0]);
                vxuFillImage(context, 0x00, images[1]);
                vxuFillImage(context, 0x00, images[2]);
                vxuFillImage(context, 0xAD, (vx_image)vxGetReferenceFromDelay(delay, -1));
                vxuFillImage(context, 0x00, (vx_image)vxGetReferenceFromDelay(delay, 0));

                status = vxProcessGraph(graph);

                if (status != VX_SUCCESS)
                {
                    ALARM("Failed to process graph!\n");
                    status = VX_ERROR_NOT_SUFFICIENT;
                }
                else
                {
                    if (vxuCheckImage(context, (vx_image)vxGetReferenceFromDelay(delay, 0), 0xBE, &errors) == VX_SUCCESS &&
                        vxuCheckImage(context, (vx_image)vxGetReferenceFromDelay(delay, -1), 0xAD, &errors) == VX_SUCCESS &&
                        vxuCheckImage(context, images[1], 0xAD, &errors) == VX_SUCCESS &&
                        vxuCheckImage(context, images[2], 0xAD, &errors) == VX_SUCCESS)
                    {
                        status = vxAgeDelay(delay);
                        if (status == VX_SUCCESS &&
                            vxuCheckImage(context, (vx_image)vxGetReferenceFromDelay(delay, 0), 0xAD, &errors) == VX_SUCCESS &&
                            vxuCheckImage(context, (vx_image)vxGetReferenceFromDelay(delay, -1), 0xBE, &errors) == VX_SUCCESS &&
                            vxuCheckImage(context, images[1], 0xAD, &errors) == VX_SUCCESS &&
                            vxuCheckImage(context, images[2], 0xAD, &errors) == VX_SUCCESS)
                        {
                            status = vxProcessGraph(graph);
                            if (status == VX_SUCCESS)
                            {
                                if (vxuCheckImage(context, (vx_image)vxGetReferenceFromDelay(delay, 0), 0xBE, &errors) == VX_SUCCESS &&
                                    vxuCheckImage(context, (vx_image)vxGetReferenceFromDelay(delay, -1), 0xBE, &errors) == VX_SUCCESS &&
                                    vxuCheckImage(context, images[1], 0xBE, &errors) == VX_SUCCESS &&
                                    vxuCheckImage(context, images[2], 0xBE, &errors) == VX_SUCCESS)
                                {
                                    ALARM("Passed!");
                                    status = VX_SUCCESS;
                                }
                                else
                                {
                                    ALARM("Aging did not work correctly!\n");
                                    status = VX_ERROR_NOT_SUFFICIENT;
                                }
                            }
                            else
                            {
                                ALARM("Failed to process graph!\n");
                                status = VX_ERROR_NOT_SUFFICIENT;
                            }
                        }
                        else
                        {
                            ALARM("Aging did not work correctly!\n");
                            status = VX_ERROR_NOT_SUFFICIENT;
                        }
                    }
                    else
                    {
                        status = VX_ERROR_NOT_SUFFICIENT;
                    }
                }
            }

            status |= vxUnloadKernels(context, "openvx-debug");
exit:
            for (i = 0; i < DEPTH_TEST; i++)
            {
                vxReleaseNode(&nodes[i]);
            }
            vxReleaseDelay(&delay);
            for (i = 0; i < DEPTH_TEST; i++)
            {
                vxReleaseImage(&images[i]);
            }
            vxReleaseGraph(&graph);
        }
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Test usage of the asynchronous interfaces.
 * \ingroup group_tests
 */
vx_status vx_test_framework_async(int argc, char *argv[])
{
    vx_status s1,s2,status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 w = 320, h = 240;
        vx_uint8 a1 = 0x5A, a2 = 0xA5;
        vx_uint32 i = 0;
        vx_uint32 errors = 0u;
        vx_image images[4] = {
                vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
                vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
                vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
                vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
        };
        vx_graph g1 = vxCreateGraph(context);
        vx_graph g2 = vxCreateGraph(context);
        status |= vxLoadKernels(context, "openvx-debug");
        if ( vxGetStatus((vx_reference)g1) == VX_SUCCESS &&
             vxGetStatus((vx_reference)g2) == VX_SUCCESS &&
             status == VX_SUCCESS)
        {
            vx_node nodes[] = {
                vxCopyImageNode(g1, images[0], images[1]),
                vxCopyImageNode(g2, images[2], images[3]),
            };
            CHECK_ALL_ITEMS(nodes, i, status, exit);
            s1 = vxVerifyGraph(g1);
            s2 = vxVerifyGraph(g2);
            printf("s1=%d, s2=%d\n",s1,s2);
            if (s1 == VX_SUCCESS && s2 == VX_SUCCESS)
            {
                vxuFillImage(context, a1, images[0]);
                vxuFillImage(context, a2, images[2]);

                s1 = vxScheduleGraph(g1);
                s2 = vxScheduleGraph(g2);
                printf("s1=%d, s2=%d\n",s1,s2);

                if (s1 == VX_SUCCESS && s2 == VX_SUCCESS)
                {
                    s2 = vxWaitGraph(g2);
                    s1 = vxWaitGraph(g1);
                    printf("s1=%d, s2=%d\n",s1,s2);
                    if ((s1 != VX_SUCCESS) || (s2 != VX_SUCCESS))
                    {
                        status = VX_ERROR_NOT_SUFFICIENT;
                    }
                    if ((vxuCheckImage(context, images[1], a1, &errors) == VX_SUCCESS) &&
                        (vxuCheckImage(context, images[3], a2, &errors) == VX_SUCCESS))
                    {
                        status = VX_SUCCESS;
                    }
                }
            }
            printf("s1=%d, s2=%d\n",s1,s2);
exit:
            vxReleaseGraph(&g1);
            vxReleaseGraph(&g2);
        }

        status |= vxUnloadKernels(context, "openvx-debug");
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Test calling a direct copy.
 * \ingroup group_tests
 */
vx_status vx_test_direct_copy_image(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_imagepatch_addressing_t addrs[] = {
                {WIDTH, HEIGHT, sizeof(vx_uint8), WIDTH * sizeof(vx_uint8), VX_SCALE_UNITY, VX_SCALE_UNITY, 1, 1},
                {WIDTH, HEIGHT, sizeof(vx_uint8), WIDTH * sizeof(vx_uint8), VX_SCALE_UNITY, VX_SCALE_UNITY, 1, 1},
                {WIDTH, HEIGHT, sizeof(vx_uint8), WIDTH * sizeof(vx_uint8), VX_SCALE_UNITY, VX_SCALE_UNITY, 1, 1},
        };
        void *src_ptrs[] = {&images[0][0][0][0][0],
                            &images[0][1][0][0][0],
                            &images[0][2][0][0][0]
        };
        void *dst_ptrs[] = {&images[1][0][0][0][0],
                            &images[1][1][0][0][0],
                            &images[1][2][0][0][0]
        };
        vx_image input = vxCreateImageFromHandle(context, VX_DF_IMAGE_YUV4, addrs, src_ptrs, VX_MEMORY_TYPE_HOST);
        vx_image output = vxCreateImageFromHandle(context, VX_DF_IMAGE_YUV4, addrs, dst_ptrs, VX_MEMORY_TYPE_HOST);
        if (input && output)
        {
            vx_uint32 numDiffs = 0;
            status = vxLoadKernels(context, "openvx-debug");
            assert(status == VX_SUCCESS);
            status = vxuFillImage(context, 0x42, input);
            assert(status == VX_SUCCESS);
            status = vxuFillImage(context, 0x00, output);
            assert(status == VX_SUCCESS);
            status = vxuCopyImage(context, input, output);
            assert(status == VX_SUCCESS);
            if (status == VX_SUCCESS)
                status = vxuCompareImages(context, input, output, &numDiffs);
            vxReleaseImage(&input);
            vxReleaseImage(&output);
            status = vxUnloadKernels(context, "openvx-debug");
            assert(status == VX_SUCCESS);
        }
        vxReleaseContext(&context);
    }
    return status;
}

/*!
 * \brief Test calling a direct copy node.
 * \ingroup group_tests
 */
vx_status vx_test_direct_copy_external_image(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
       vx_uint32 width = 640;
        vx_uint32 height = 480;
        vx_image input = vxCreateImage(context, width, height, VX_DF_IMAGE_RGB);
        vx_image output = vxCreateImage(context, width, height, VX_DF_IMAGE_RGB);
        if (input && output)
        {
            vx_uint32 value = 0x225533;
            vx_uint32 errors = 0xFFFFFFFF;
            status = vxLoadKernels(context, "openvx-debug");
            assert(status == VX_SUCCESS);
            status |= vxuFillImage(context, value, input);
            assert(status == VX_SUCCESS);
            status |= vxuFillImage(context, 0x00, output);
            assert(status == VX_SUCCESS);
            status |= vxuCopyImage(context, input, output);
            assert(status == VX_SUCCESS);
            status |= vxuCheckImage(context, output, value, &errors);
            if ((status != VX_SUCCESS) && (errors > 0))
            {
                status = VX_ERROR_NOT_SUFFICIENT;
            }
            vxReleaseImage(&input);
            vxReleaseImage(&output);
            status = vxUnloadKernels(context, "openvx-debug");
            assert(status == VX_SUCCESS);
        }
        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_test_graph_channels_yuv(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 i = 0, w = 640, h = 480;
        vx_image images[] = {
            vxCreateImage(context, w, h, VX_DF_IMAGE_RGB),    /* 0: rgb */
            vxCreateImage(context, w, h, VX_DF_IMAGE_IYUV),   /* 1: yuv */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),     /* 2: luma */
            vxCreateImage(context, w/2, h/2, VX_DF_IMAGE_U8), /* 3: u channel */
            vxCreateImage(context, w/2, h/2, VX_DF_IMAGE_U8), /* 4: v channel */
        };
        CHECK_ALL_ITEMS(images, i, status, exit);
        status = vxLoadKernels(context, "openvx-debug");
        if (status == VX_SUCCESS)
        {
            vx_graph graph = vxCreateGraph(context);
            if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
            {
                vx_node nodes[] = {
                    vxFReadImageNode(graph, "colorbars_640x480_I444.rgb", images[0]),
                    vxColorConvertNode(graph, images[0], images[1]),
                    vxFWriteImageNode(graph, images[1], "oiyuv_640x480_P420.yuv"),
                    vxChannelExtractNode(graph, images[1], VX_CHANNEL_Y, images[2]),
                    vxChannelExtractNode(graph, images[1], VX_CHANNEL_U, images[3]),
                    vxChannelExtractNode(graph, images[1], VX_CHANNEL_V, images[4]),
                    vxFWriteImageNode(graph, images[2], "oy_640x480_P400.bw"),
                    vxFWriteImageNode(graph, images[3], "ou_320x240_P400.bw"),
                    vxFWriteImageNode(graph, images[4], "ov_320x240_P400.bw"),
                };
                CHECK_ALL_ITEMS(nodes, i, status, exit);
                if (status == VX_SUCCESS)
                {
                    status = vxVerifyGraph(graph);
                    if (status == VX_SUCCESS)
                    {
                        status = vxProcessGraph(graph);
                    }
                }
                for (i = 0; i < dimof(nodes); i++)
                {
                    vxReleaseNode(&nodes[i]);
                }
                vxReleaseGraph(&graph);
            }
            status |= vxUnloadKernels(context, "openvx-debug");
        }
        for (i = 0; i < dimof(images); i++)
        {
            vxReleaseImage(&images[i]);
        }
exit:
        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_test_graph_bikegray(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vxRegisterHelperAsLogReader(context);
        vx_uint32 i = 0, w = 640, h = 480, w2 = 300, h2 = 200, x = 0, y = 0;
        vx_uint32 range = 256, windows = 16;
        vx_image images[] = {
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 0: luma */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 1: scaled */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_S16),             /* 2: grad_x */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_S16),             /* 3: grad_y */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_S16),             /* 4: mag */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 5: phase */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 6: LUT out */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 7: AbsDiff */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 8: Threshold */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U32),             /* 9: Integral */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 10: Eroded */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 11: Dilated */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 12: Median */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 13: Box */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_S16),             /* 14: UpDepth */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 15: Canny */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 16: EqualizeHistogram */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),              /* 17: DownDepth Gy */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),                /* 18: Remapped */
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),                /* 19: Remapped 2*/
            vxCreateImage(context, w2, h2, VX_DF_IMAGE_U8),                /* 20: Diff */
        };
        vx_lut lut = vxCreateLUT(context, VX_TYPE_UINT8, 256);
        vx_uint8 *tmp = NULL;
        vx_int32 *histogram = NULL;
        vx_distribution dist = vxCreateDistribution(context, windows, 0, range);
        vx_float32 mean = 0.0f, stddev = 0.0f;
        vx_scalar s_mean = vxCreateScalar(context, VX_TYPE_FLOAT32, &mean);
        vx_scalar s_stddev = vxCreateScalar(context, VX_TYPE_FLOAT32, &stddev);
        vx_threshold thresh = vxCreateThreshold(context, VX_THRESHOLD_TYPE_BINARY, VX_TYPE_UINT8);
        vx_int32 lo = 140;
        vx_scalar minVal = vxCreateScalar(context, VX_TYPE_UINT8, NULL);
        vx_scalar maxVal = vxCreateScalar(context, VX_TYPE_UINT8, NULL);
        vx_array minLoc = vxCreateArray(context, VX_TYPE_COORDINATES2D, 1);
        vx_array maxLoc = vxCreateArray(context, VX_TYPE_COORDINATES2D, 1);
        vx_threshold hyst = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8);
        vx_int32 lower = 40, upper = 250;
        vx_enum policy = VX_CONVERT_POLICY_SATURATE;
        vx_int32 shift = 7;
        vx_scalar sshift = vxCreateScalar(context, VX_TYPE_INT32, &shift);
        vx_int32 noshift = 0;
        vx_scalar snoshift = vxCreateScalar(context, VX_TYPE_INT32, &noshift);
        vx_remap table = vxCreateRemap(context, w, h, w2, h2);
        vx_float32 dx = (vx_float32)w/w2, dy = (vx_float32)h/h2;

        CHECK_ALL_ITEMS(images, i, status, exit);

        status = vxAccessLUT(lut, (void **)&tmp, VX_WRITE_ONLY);
        if (status == VX_SUCCESS)
        {
            for (i = 0; i < range; i++)
            {
                vx_uint32 g = (vx_uint32)pow(i,1/0.93);
                tmp[i] = (vx_uint8)(g >= range ? (range - 1): g);
            }
            status = vxCommitLUT(lut, tmp);
        }

        /* create the remapping for "image scaling" using remap */
        for (y = 0; y < h2; y++)
        {
            for (x = 0; x < w2; x++)
            {
                vx_float32 nx = dx*x;
                vx_float32 ny = dy*y;
                //printf("Setting point %lu, %lu from %lf, %lf (dx,dy=%lf,%lf)\n", x, y, nx, ny, dx, dy);
                vxSetRemapPoint(table, x, y, nx, ny);
            }
        }

        status |= vxSetThresholdAttribute(thresh, VX_THRESHOLD_THRESHOLD_VALUE, &lo, sizeof(lo));
        status |= vxSetThresholdAttribute(hyst, VX_THRESHOLD_THRESHOLD_LOWER, &lower, sizeof(lower));
        status |= vxSetThresholdAttribute(hyst, VX_THRESHOLD_THRESHOLD_UPPER, &upper, sizeof(upper));
        status |= vxLoadKernels(context, "openvx-debug");
        if (status == VX_SUCCESS)
        {
            vx_graph graph = vxCreateGraph(context);
            if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
            {
                vx_node nodes[] = {
                    vxFReadImageNode(graph, "bikegray_640x480.pgm", images[0]),
                    vxMedian3x3Node(graph, images[0], images[12]),
                    vxBox3x3Node(graph, images[0], images[13]),
                    vxScaleImageNode(graph, images[0], images[1], VX_INTERPOLATION_AREA),
                    vxTableLookupNode(graph, images[1], lut, images[6]),
                    vxHistogramNode(graph, images[6], dist),
                    vxSobel3x3Node(graph, images[1], images[2], images[3]),
                    vxMagnitudeNode(graph, images[2], images[3], images[4]),
                    vxPhaseNode(graph, images[2], images[3], images[5]),
                    vxAbsDiffNode(graph, images[6], images[1], images[7]),
                    vxConvertDepthNode(graph, images[7], images[14], policy, sshift),
                    vxMeanStdDevNode(graph, images[7], s_mean, s_stddev),
                    vxThresholdNode(graph, images[6], thresh, images[8]),
                    vxIntegralImageNode(graph, images[7], images[9]),
                    vxErode3x3Node(graph, images[8], images[10]),
                    vxDilate3x3Node(graph, images[8], images[11]),
                    vxMinMaxLocNode(graph, images[7], minVal, maxVal, minLoc, maxLoc, 0, 0),
                    vxCannyEdgeDetectorNode(graph, images[0], hyst, 3, VX_NORM_L1, images[15]),
                    vxEqualizeHistNode(graph, images[0], images[16]),
                    vxConvertDepthNode(graph, images[3], images[17], policy, snoshift),
                    vxRemapNode(graph, images[0], table, VX_INTERPOLATION_NEAREST_NEIGHBOR, images[18]),
                    vxRemapNode(graph, images[0], table, VX_INTERPOLATION_BILINEAR, images[19]),
                    vxAbsDiffNode(graph, images[18], images[19], images[20]),

                    vxFWriteImageNode(graph, images[0], "obikegray_640x480_P400.pgm"),
                    vxFWriteImageNode(graph, images[1], "obikegray_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[2], "obikegradh_300x200_P400_-16b.bw"),
                    vxFWriteImageNode(graph, images[3], "obikegradv_300x200_P400_-16b.bw"),
                    vxFWriteImageNode(graph, images[4], "obikemag_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[5], "obikeatan_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[6], "obikeluty_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[7], "obikediff_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[8], "obikethsh_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[9], "obikesums_600x200_P400_16b.bw"),
                    vxFWriteImageNode(graph, images[10], "obikeerod_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[11], "obikedilt_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[12], "obikemed_640x480_P400.pgm"),
                    vxFWriteImageNode(graph, images[13], "obikeavg_640x480_P400.pgm"),
                    vxFWriteImageNode(graph, images[14], "obikediff_300x200_P400_16b.bw"),
                    vxFWriteImageNode(graph, images[15], "obikecanny_640x480_P400.pgm"),
                    vxFWriteImageNode(graph, images[16], "obikeeqhist_640x480_P400.pgm"),
                    vxFWriteImageNode(graph, images[17], "obikegrady8_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[18], "obikeremap_nn_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[19], "obikeremap_bi_300x200_P400.pgm"),
                    vxFWriteImageNode(graph, images[20], "obikeremap_ab_300x200_P400.pgm"),
                };
                vx_print_log((vx_reference)context);
                CHECK_ALL_ITEMS(nodes, i, status, exit);
                if (status == VX_SUCCESS)
                {
                    status = vxVerifyGraph(graph);
                    vx_print_log((vx_reference)graph);
                    if (status == VX_SUCCESS)
                    {
                        status = vxProcessGraph(graph);
                        assert(status == VX_SUCCESS);
                    }
                    else
                    {
                        vx_print_log((vx_reference)graph);
                    }
                    if (status == VX_SUCCESS)
                    {
                        vx_coordinates2d_t min_l, *p_min_l = &min_l;
                        vx_coordinates2d_t max_l, *p_max_l = &max_l;
                        vx_size stride = 0;
                        vx_uint8 min_v = 255;
                        vx_uint8 max_v = 0;
                        vx_map_id map_id = 0;

                        vxAccessArrayRange(minLoc, 0, 1, &stride, (void **)&p_min_l, VX_READ_ONLY);
                        vxCommitArrayRange(minLoc, 0, 0, p_min_l);
                        vxAccessArrayRange(maxLoc, 0, 1, &stride, (void **)&p_max_l, VX_READ_ONLY);
                        vxCommitArrayRange(maxLoc, 0, 0, p_max_l);

                        vxCopyScalar(minVal, &min_v, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                        vxCopyScalar(maxVal, &max_v, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                        printf("Min Value in AbsDiff = %u, at %d,%d\n", min_v, min_l.x, min_l.y);
                        printf("Max Value in AbsDiff = %u, at %d,%d\n", max_v, max_l.x, max_l.y);

                        vxCopyScalar(s_mean, &mean, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                        vxCopyScalar(s_stddev, &stddev, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

                        printf("AbsDiff Mean = %lf\n", mean);
                        printf("AbsDiff Stddev = %lf\n", stddev);

                        vxMapDistribution(dist, &map_id, (void**)&histogram, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
                        for (i = 0; i < windows; i++)
                        {
                            printf("histogram[%u] = %d\n", i, histogram[i]);
                        }
                        vxUnmapDistribution(dist, map_id);
                    }
                    else
                    {
                        printf("Graph failed (%d)\n", status);
                        for (i = 0; i < dimof(nodes); i++)
                        {
                            status = VX_SUCCESS;
                            vxQueryNode(nodes[i], VX_NODE_STATUS, &status, sizeof(status));
                            if (status != VX_SUCCESS)
                            {
                                printf("nodes[%u] failed with %d\n", i, status);
                            }
                        }
                        status = VX_ERROR_NOT_SUFFICIENT;
                    }
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        vxReleaseNode(&nodes[i]);
                    }
                }
                vxReleaseGraph(&graph);
            }
            status |= vxUnloadKernels(context, "openvx-debug");
        }
        for (i = 0; i < dimof(images); i++)
        {
            vxReleaseImage(&images[i]);
        }
exit:
        //vx_print_log((vx_reference)context);
        /* Deregister log callback */
        vxRegisterLogCallback(context, NULL, vx_false_e);
        vxReleaseRemap(&table);
        vxReleaseScalar(&sshift);
        vxReleaseScalar(&snoshift);
        vxReleaseThreshold(&hyst);
        vxReleaseThreshold(&thresh);
        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_test_graph_lena(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 i = 0, w = 512, h = 512;
        vx_image images[] = {
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 0: BW */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 1: Median */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 2: Box */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 3: Gaussian */
            vxCreateImage(context, w, h, VX_DF_IMAGE_S16),               /* 4: Custom */
            vxCreateImage(context, w, h, VX_DF_IMAGE_S16),               /* 5: Custom */
            vxCreateImage(context, w, h, VX_DF_IMAGE_S16),               /* 6: Mag */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 7: Laplacian */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 8: Affine */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),                /* 9: Perspective */
        };
        vx_convolution conv[] = {
            vxCreateConvolution(context, 3, 3),
            vxCreateConvolution(context, 3, 3),
        };
        vx_pyramid pyramid = vxCreatePyramid(context, 4, VX_SCALE_PYRAMID_HALF, w, h, VX_DF_IMAGE_U8);
        vx_matrix affine = vxCreateMatrix(context, VX_TYPE_FLOAT32, 2, 3);
        vx_matrix perspective = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3);
        /* just some non-sense filters */
        vx_int16 mat1[3][3] = {
            { 1, 0,-1},
            { 3, 0,-3},
            { 1, 0,-1},
        };
        vx_int16 mat2[3][3] = {
            { 1, 3, 1},
            { 0, 0, 0},
            {-1,-3,-1},
        };
        /* rotate and mirror, and scale */
       vx_float32 mat4[3][3] = {
            {0, 1, 0},
            {1, 0, 0},
            {0, 0, 0.5},
        };

        CHECK_ALL_ITEMS(images, i, status, exit);
        vxCopyConvolutionCoefficients(conv[0], (vx_int16 *)mat1, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
        vxCopyConvolutionCoefficients(conv[1], (vx_int16 *)mat2, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
        vxSetAffineRotationMatrix(affine, 45.0, 0.5, (vx_float32)w/2, (vx_float32)h/2);
        vxCopyMatrix(perspective, mat4, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

        status |= vxLoadKernels(context, "openvx-debug");
        status |= vxLoadKernels(context, "openvx-extras");

        if (status == VX_SUCCESS)
        {
            vx_perf_t perf;
            vx_graph graph = vxCreateGraph(context);
            vxDirective((vx_reference)context, VX_DIRECTIVE_ENABLE_PERFORMANCE);
            if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
            {
                vx_node nodes[] = {
                    vxFReadImageNode(graph, "lena_512x512.pgm", images[0]),
                    vxMedian3x3Node(graph, images[0], images[1]),
                    vxBox3x3Node(graph, images[0], images[2]),
                    vxGaussian3x3Node(graph, images[0], images[3]),
                    vxConvolveNode(graph, images[3], conv[0], images[4]),
                    vxConvolveNode(graph, images[3], conv[1], images[5]),
                    vxMagnitudeNode(graph, images[4], images[5], images[6]),
                    vxLaplacian3x3Node(graph, images[0], images[7]),
                    vxGaussianPyramidNode(graph, images[0], pyramid),
                    vxWarpAffineNode(graph, images[0], affine, VX_INTERPOLATION_NEAREST_NEIGHBOR, images[8]),
                    vxWarpPerspectiveNode(graph, images[0], perspective, VX_INTERPOLATION_NEAREST_NEIGHBOR, images[9]),
                    vxFWriteImageNode(graph, images[1], "olenamed_512x512_P400.bw"),
                    vxFWriteImageNode(graph, images[2], "olenaavg_512x512_P400.bw"),
                    vxFWriteImageNode(graph, images[3], "olenagau_512x512_P400.bw"),
                    vxFWriteImageNode(graph, images[4], "olenacus1_512x512_P400_-16b.bw"),
                    vxFWriteImageNode(graph, images[5], "olenacus2_512x512_P400_-16b.bw"),
                    vxFWriteImageNode(graph, images[6], "olenamag_512x512_P400.bw"),
                    vxFWriteImageNode(graph, images[7], "olenalapl_512x512_P400.bw"),
                    vxFWriteImageNode(graph, images[8], "olenaaffine_512x512_P400.bw"),
                    vxFWriteImageNode(graph, images[9], "olenaperspec_512x512_P400.bw"),
                };
                CHECK_ALL_ITEMS(nodes, i, status, exit);
                if (status == VX_SUCCESS)
                {
                    status = vxVerifyGraph(graph);
                    if (status == VX_SUCCESS)
                    {
                        // in testing this graph goes from ~55ms to ~(28-30)ms with SMP.
                        //vxHint(context, (vx_reference)graph, VX_HINT_SERIALIZE);

                        // not working quite yet.
                        //vxExportGraphToDot(graph, "lena.dot", vx_false_e);
                        status = vxProcessGraph(graph);
                    }
                    if (status == VX_SUCCESS)
                    {
                        vxuFWriteImage(context, vxGetPyramidLevel(pyramid, 0), "olenapyr_512x512_P400.bw");
                        vxuFWriteImage(context, vxGetPyramidLevel(pyramid, 1), "olenapyr_256x256_P400.bw");
                        vxuFWriteImage(context, vxGetPyramidLevel(pyramid, 2), "olenapyr_128x128_P400.bw");
                        vxuFWriteImage(context, vxGetPyramidLevel(pyramid, 3), "olenapyr_64x64_P400.bw");
                    }
                    else
                    {
                        printf("Graph failed (%d)\n", status);
                        for (i = 0; i < dimof(nodes); i++)
                        {
                            status = VX_SUCCESS;
                            vxQueryNode(nodes[i], VX_NODE_STATUS, &status, sizeof(status));
                            if (status != VX_SUCCESS)
                            {
                                printf("nodes[%u] failed with %d\n", i, status);
                            }
                        }
                        status = VX_ERROR_NOT_SUFFICIENT;
                    }
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        vxQueryNode(nodes[i], VX_NODE_PERFORMANCE, &perf, sizeof(perf));
                        printf("Nodes[%u] average exec: %0.3lfms\n", i, (vx_float32)perf.avg/10000000.0f);
                        vxReleaseNode(&nodes[i]);
                    }
                }
                vxQueryGraph(graph, VX_GRAPH_PERFORMANCE, &perf, sizeof(perf));
                printf("Graph average exec: %0.3lfms\n", (vx_float32)perf.avg/10000000.0f);
                vxReleaseGraph(&graph);
            }
            status |= vxUnloadKernels(context, "openvx-extras");
            status |= vxUnloadKernels(context, "openvx-debug");
        }
        vxReleasePyramid(&pyramid);
        vxReleaseMatrix(&affine);
        vxReleaseMatrix(&perspective);
        for (i = 0; i < dimof(images); i++)
        {
            vxReleaseImage(&images[i]);
        }
exit:
        //vx_print_log((vx_reference)context);
        vxReleaseContext(&context);
    }
    return status;
}


vx_status vx_test_graph_channels_rgb(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 i = 0, diffs = 0u;
        vx_uint32 w = 640, h = 480;
        vx_image images[] = {
            vxCreateImage(context, w, h, VX_DF_IMAGE_RGB),    /* 0: rgb */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),     /* 1: r */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),     /* 2: g */
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),     /* 3: b */
            vxCreateImage(context, w, h, VX_DF_IMAGE_RGB),    /* 4: rgb */
        };
        status = VX_SUCCESS;
        for (i = 0; i < dimof(images); i++)
        {
            if (images[i] == 0)
            {
                status = VX_ERROR_NOT_SUFFICIENT;
            }
        }
        status |= vxLoadKernels(context, "openvx-debug");
        if (status == VX_SUCCESS)
        {
            vx_graph graph = vxCreateGraph(context);
            if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
            {
                vx_node nodes[] = {
                    vxFReadImageNode(graph, "colorbars_640x480_I444.rgb", images[0]),
                    vxChannelExtractNode(graph, images[0], VX_CHANNEL_R, images[1]),
                    vxChannelExtractNode(graph, images[0], VX_CHANNEL_G, images[2]),
                    vxChannelExtractNode(graph, images[0], VX_CHANNEL_B, images[3]),
                    vxFWriteImageNode(graph, images[1], "or_640x480_P400.bw"),
                    vxFWriteImageNode(graph, images[2], "og_640x480_P400.bw"),
                    vxFWriteImageNode(graph, images[3], "ob_640x480_P400.bw"),
                    vxChannelCombineNode(graph, images[1], images[2], images[3], 0, images[4]),
                    vxFWriteImageNode(graph, images[4], "ocolorbars2_640x480_I444.rgb"),
                };
                //CHECK_ALL_ITEMS(nodes, i, status, exit);
                if (status == VX_SUCCESS)
                {
                    status = vxVerifyGraph(graph);
                    if (status == VX_SUCCESS)
                    {
                        status = vxProcessGraph(graph);
                    }
                    if (status == VX_SUCCESS)
                    {
                        // make sure images[0] == images[4]
                        status = vxuCompareImages(context, images[0], images[4], &diffs);
                        printf("Found %u differences between images (status=%d)\n", diffs, status);
                    }
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        vxReleaseNode(&nodes[i]);
                    }
                }
                vxReleaseGraph(&graph);
            }
            status |= vxUnloadKernels(context, "openvx-debug");
        }
        for (i = 0; i < dimof(images); i++)
        {
            vxReleaseImage(&images[i]);
        }
        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_test_graph_accum(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 i = 0, w = 640, h = 480, r = 8;
        vx_image images[] = {
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
            vxCreateImage(context, w, h, VX_DF_IMAGE_S16),
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
            vxCreateImage(context, w, h, VX_DF_IMAGE_S16),
        };
        vx_float32 alpha = 0.5f;
        vx_uint32 shift = 4;
        vx_scalar scalars[] = {
                vxCreateScalar(context, VX_TYPE_FLOAT32, &alpha),
                vxCreateScalar(context, VX_TYPE_UINT32, &shift),
        };
        CHECK_ALL_ITEMS(images, i, status, exit);
        status |= vxLoadKernels(context, "openvx-debug");
        if (status == VX_SUCCESS)
        {
            vx_graph graph = vxCreateGraph(context);
            if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
            {
                vx_node nodes[] = {
                    vxFReadImageNode(graph, "bikegray_640x480.pgm", images[0]),
                    vxAccumulateImageNode(graph, images[0], images[1]),
                    vxAccumulateWeightedImageNode(graph, images[0], scalars[0], images[2]),
                    vxAccumulateSquareImageNode(graph, images[0], scalars[1], images[3]),
                    vxFWriteImageNode(graph, images[1], "obikeaccu_640x480_P400_16b.bw"),
                    vxFWriteImageNode(graph, images[2], "obikeaccw_640x480_P400_16b.bw"),
                    vxFWriteImageNode(graph, images[3], "obikeaccq_640x480_P400_16b.bw"),
                };
                CHECK_ALL_ITEMS(nodes, i, status, exit);
                if (status == VX_SUCCESS)
                {
                    status = vxVerifyGraph(graph);
                    if (status == VX_SUCCESS)
                    {
                        while (r-- && status == VX_SUCCESS)
                            status = vxProcessGraph(graph);
                    }
                    if (status == VX_SUCCESS)
                    {
                        /* add a node which should cause the graph to be re-verified and will cause a failure. */
                        vx_node node = vxAccumulateImageNode(graph, images[0], images[1]);
                        if (node == 0)
                        {
                            printf("Failed to create node to break graph!\n");
                        }
                        if (vxIsGraphVerified(graph) == vx_true_e)
                        {
                            return VX_ERROR_NOT_SUFFICIENT;
                        }
                        status = vxVerifyGraph(graph);
                        if (status == VX_SUCCESS)
                        {
                            printf("Failed to fail multiple writers check!\n");
                            status = VX_ERROR_NOT_SUFFICIENT;
                        }
                        else if (status == VX_ERROR_MULTIPLE_WRITERS)
                        {
                            printf("Multiple writers failed with %d!\n", status);
                            status = VX_SUCCESS;
                        }
                        else
                        {
                            /* wrong error code! */
                            status = VX_ERROR_NOT_SUFFICIENT;
                        }
                    }
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        vxReleaseNode(&nodes[i]);
                    }
                }
                vxReleaseGraph(&graph);
            }
            status |= vxUnloadKernels(context, "openvx-debug");
        }
        for (i = 0; i < dimof(images); i++)
        {
            vxReleaseImage(&images[i]);
        }
        for (i = 0; i < dimof(scalars); i++)
        {
            vxReleaseScalar(&scalars[i]);
        }
exit:
        vxReleaseContext(&context);
    }
    return status;
}

static vx_uint32 b_and(vx_uint32 a, vx_uint32 b) { return a & b; }
static vx_uint32 b_or(vx_uint32 a, vx_uint32 b) { return a | b; }
static vx_uint32 b_xor(vx_uint32 a, vx_uint32 b) { return a ^ b; }
static vx_uint32 b_not_1st(vx_uint32 a, vx_uint32 b) { (void)b; return ~a; }
static vx_uint32 b_not_2nd(vx_uint32 a, vx_uint32 b) { (void)a; return ~b; }
static vx_uint32 b_nand(vx_uint32 a, vx_uint32 b) { return ~(a & b); }
static vx_uint32 b_nor(vx_uint32 a, vx_uint32 b) { return ~(a | b); }
static vx_uint32 b_xorn(vx_uint32 a, vx_uint32 b) { return a ^ ~b; }

#undef OPX
#define OPX(x) {b_ ## x, #x}

typedef struct _subtest_t {
    vx_uint32 (*op)(vx_uint32, vx_uint32);
    const char *s;
} subtest_t;

/*!
 * \brief Test for the bitwise operators.
 * \ingroup group_tests
 */
vx_status vx_test_graph_bitwise(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 i = 0, width = 640, height = 480;
        vx_graph graph = 0;
        vx_image images[] = {
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* in1 */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* in2 */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* and */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* or */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* xor */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* not (on first image of two) */
            /*
             * The following are just trivial sequence tests; no node combination
             * can be expected, as the intermediate results are inspected.
             */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* not (on second image of two) */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* nand */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* nor */
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8), /* xorn */
        };

        CHECK_ALL_ITEMS(images, i, status, release_context);

        status = vxLoadKernels(context, "openvx-debug");
        if (status != VX_SUCCESS)
            FAIL(release_context, "can't load debug extensions");

        graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_node nodes[] = {
                vxAndNode(graph, images[0], images[1], images[2]),
                vxOrNode(graph, images[0], images[1], images[3]),
                vxXorNode(graph, images[0], images[1], images[4]),
                vxNotNode(graph, images[0], images[5]),

                vxNotNode(graph, images[1], images[6]),
                vxNotNode(graph, images[2], images[7]),
                vxNotNode(graph, images[3], images[8]),
                vxXorNode(graph, images[0], images[6], images[9]),
            };

            /*
             * Let's avoid putting 0 at (0, 0): let's offset the first
             * pixel-values. The start values are otherwise somewhat random.
             */
            vx_uint8 v0_start = 42, v1_start = 256 - 77;
            vx_uint8 v0 = v0_start, v1 = v1_start;

            vx_uint32 x, y;
            vx_rectangle_t rect = {0, 0, width, height};
            vx_imagepatch_addressing_t image0_addr;
            vx_imagepatch_addressing_t image1_addr;
            void *base0 = NULL;
            void *base1 = NULL;
            /* The images to test are now at images[2], ... images[N - 1]. */
            subtest_t subtests[] = {
                OPX(and),
                OPX(or),
                OPX(xor),
                OPX(not_1st),
                OPX(not_2nd),
                OPX(nand),
                OPX(nor),
                OPX(xorn)
            };

            status = vxAccessImagePatch(images[0], &rect, 0, &image0_addr, &base0, VX_WRITE_ONLY);
            status |= vxAccessImagePatch(images[1], &rect, 0, &image1_addr, &base1, VX_WRITE_ONLY);
            if (status != VX_SUCCESS)
                FAIL(release_rect, "couldn't get image patch");
            CHECK_ALL_ITEMS(nodes, i, status, release_graph);

            /*
             * Fill the source images. As long as each image size (i.e. width*height) is
             * >= 256*256, these tests will be exhaustive wrt. the domain of the core
             * operations.
             */
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    vx_uint8 *img0 = vxFormatImagePatchAddress2d(base0, x, y, &image0_addr);
                    vx_uint8 *img1 = vxFormatImagePatchAddress2d(base1, x, y, &image1_addr);

                    *img0 = v0++;
                    *img1 = v1;

                    if (v0 == v0_start)
                        v1++;
                }
            }
            status = vxCommitImagePatch(images[0], &rect, 0, &image0_addr, base0);
            status = vxCommitImagePatch(images[1], &rect, 0, &image1_addr, base1);

            status = vxVerifyGraph(graph);
            if (status != VX_SUCCESS)
                FAIL(release_rect, "graph verification failed");

            status = vxProcessGraph(graph);
            if (status != VX_SUCCESS)
                FAIL(release_rect, "graph processing failed");


            for (i = 0; i < dimof(subtests); i++)
            {
                vx_imagepatch_addressing_t image_addr;
                void *base = NULL;
                v0 = v0_start;
                v1 = v1_start;

                status = vxAccessImagePatch(images[i + 2], &rect, 0, &image_addr, &base, VX_READ_ONLY);
                if (status != VX_SUCCESS)
                    VFAIL(release_rect, "couldn't get image patch for %s-image", subtests[i].s);

                for (y = 0; y < height; y++)
                {
                    for (x = 0; x < width; x++)
                    {
                        vx_uint8 *img = vxFormatImagePatchAddress2d(base, x, y, &image_addr);
                        vx_uint8 ref = subtests[i].op(v0, v1);

                        if (ref != *img)
                            VFAIL(release_rect,
                                  "wrong result at (x, y) = (%d, %d) for bitwise %s(0x%x, 0x%x): 0x%x should be 0x%x",
                                  x, y, subtests[i].s, v0, v1, *img, ref);

                        v0++;
                        if (v0 == v0_start)
                            v1++;
                    }
                }

                status = vxCommitImagePatch(images[i + 2], 0, 0, &image_addr, base);

            }

release_rect:
            for (i = 0; i < dimof(nodes); i++)
                vxReleaseNode(&nodes[i]);

release_graph:
            vxReleaseGraph(&graph);
        }
        for (i = 0; i < dimof(images); i++)
            vxReleaseImage(&images[i]);

        status |= vxUnloadKernels(context, "openvx-debug");

release_context:
        vxReleaseContext(&context);
    }
    return status;
}

/*
 * Create an image of (width, height) in format. Fill the image,
 * covering different values, changing values no more often than a set
 * period. Use seed when initializing values.
 */
static vx_image vx_create_image_valuecovering(vx_context context, vx_df_image format,
                                              vx_uint32 width, vx_uint32 height,
                                              vx_uint32 seed,
                                              vx_uint32 period)
{
    vx_image image = vxCreateImage(context, width, height, format);
    vx_rectangle_t rect = {0, 0, width, height};
    vx_imagepatch_addressing_t addr;
    void *base = NULL;
    vx_status status = vxAccessImagePatch(image, &rect, 0, &addr, &base, VX_READ_AND_WRITE);
    vx_uint32 x, y;
    vx_uint32 period_counter = 0;

    if (status != VX_SUCCESS)
        goto release_rect;

    if (format == VX_DF_IMAGE_U8)
    {
        /*
         * Assume we're going to iterate over all values for this image
         * and combinations together with another VX_DF_IMAGE_U8 image; no need
         * for randomness.
         */
        vx_uint8 val = seed & 255;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                vx_uint8 *img = vxFormatImagePatchAddress2d(base, x, y, &addr);

                *img = val;
                if (++period_counter == period)
                {
                    period_counter = 0;
                    val++;
                }
            }
        }
    }
    else if (format == VX_DF_IMAGE_S16)
    {
        vx_int16 val = 0;
        /*
         * There are probably not enough pixels in the image to cover
         * all combinations of VX_DF_IMAGE_S16 together with (even)
         * VX_DF_IMAGE_U8, so go for something a little more random. We don't
         * need a high-quality PRNG, whatever is delivered as part of a
         * C99 implementation is sufficient.
         */
        srand(seed);

        /* The subtraction only makes a difference if RAND_MAX < 65535. */
        val = (vx_int16)(rand() - 32768);

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                vx_int16 *img = vxFormatImagePatchAddress2d(base, x, y, &addr);

                *img = val;

                if (++period_counter == period)
                {
                    period_counter = 0;
                    val = rand();
                }
            }
        }
    }
    else /* Unsupported; missing else-if clause above. */
    {
        vxReleaseImage(&image);
        image = 0;
    }

release_rect:
    vxCommitImagePatch(image, &rect, 0, &addr, base);
    return image;
}

static vx_int32 a_apply_policy(vx_int32 raw_result, vx_df_image destformat, enum vx_convert_policy_e policy)
{
    vx_int32 tmp;

    if (policy == VX_CONVERT_POLICY_SATURATE)
    {
        vx_int32 max = destformat == VX_DF_IMAGE_U8 ? UINT8_MAX : INT16_MAX;
        vx_int32 min = destformat == VX_DF_IMAGE_U8 ? 0 : INT16_MIN;

        if (raw_result > max)
            tmp = max;
        else if (raw_result < min)
            tmp = min;
        else
            tmp = raw_result;
    } else
        tmp = raw_result;

    return destformat == VX_DF_IMAGE_U8 ? (vx_uint8)tmp : (vx_int16)tmp;
}

static vx_int32 a_mult(vx_int32 a, vx_int32 b, vx_float32 scale,
                       vx_df_image destformat, enum vx_convert_policy_e policy)
{
    vx_int32 primary_product = a * b;
    vx_float64 scaled_raw_result = scale * (vx_float64)primary_product;
    vx_int32 inttyped_raw_result = (vx_int32)scaled_raw_result;
    vx_int32 policy_result = a_apply_policy(inttyped_raw_result, destformat, policy);
    return policy_result;
}

static vx_int32 a_add(vx_int32 a, vx_int32 b, vx_float32 scale,
                      vx_df_image destformat, enum vx_convert_policy_e policy)
{
    vx_int32 sum = a + b;
    vx_int32 policy_result = a_apply_policy(sum, destformat, policy);
    scale += 0.0; /* Avoid compiler warning about unused parameter. */
    return policy_result;
}

static vx_int32 a_sub(vx_int32 a, vx_int32 b, vx_float32 scale,
                      vx_df_image destformat, enum vx_convert_policy_e policy)
{
    vx_int32 difference = a - b;
    vx_int32 policy_result = a_apply_policy(difference, destformat, policy);
    scale += 0.0; /* Avoid compiler warning about unused parameter. */
    return policy_result;
}

/*!
 * \brief Test for the pixelwise arithmetic.
 * \ingroup group_tests
 */
vx_status vx_test_graph_arit(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    /* Let's just test a single representative size here. */
    vx_uint32 width = 640, height = 480;
    vx_rectangle_t rect = {0, 0, width, height};
    (void)argc;
    (void)argv;

    /*
     * Iterate over all combinations of types and overflow policies;
     * skip the invalid ones (or should they be attempted with expected
     * errors verified)?  For the floating-point "scale" parameter,
     * iterate over a list of representative values.
     */
    static const vx_float32 test_scales[] = { 0, 0.125f, 0.25f, 0.5f, 1.0f, 1.25f, 1.5f, 2.0f };

    /* Testing is made somewhat simpler by using the direct calls. */
    static const struct {
        vx_int32 (*refop)(vx_int32, vx_int32, vx_float32, vx_df_image, enum vx_convert_policy_e);
        vx_status(VX_CALLBACK *operate_image)(vx_context, vx_image, vx_image, vx_enum, vx_image);
        int n_scales;
        const char *name;
    } op_tests[] = {
        /*
         * A bit of simplification; using the same test-fixture despite
         * "multiply" also having a scaling parameter. The difference is
         * keyed on n_scales == 1, but only in part; see code below.
         */
        { a_add, vxuAdd, 1, "vxuAdd" },
        { a_sub, vxuSubtract, 1, "vxuSubtract" },
        { a_mult, NULL, dimof(test_scales), "vxuMult" }
    };

    /*
     * Don't iterate over any enums themselves: we can't know without
     * peeking, their individual orders or indeed whether they're
     * consecutive.
     */
    static const enum vx_convert_policy_e test_policies[] = {
        VX_CONVERT_POLICY_WRAP, VX_CONVERT_POLICY_SATURATE
    };
    static const enum vx_round_policy_e test_rounding_policies[] = {
        VX_ROUND_POLICY_TO_ZERO, VX_ROUND_POLICY_TO_NEAREST_EVEN,
    };

    static const vx_df_image test_formats[] = {
        VX_DF_IMAGE_U8, VX_DF_IMAGE_S16
    };
    vx_image src0_image = 0, src1_image = 0, dest_image = 0;
    int ipolicy;

    if (!context)
        return status;

    status = vxLoadKernels(context, "openvx-debug");
    if (status != VX_SUCCESS)
        FAIL(release_context, "can't load debug extensions");

    for (ipolicy = 0; ipolicy < (int)dimof(test_policies); ipolicy++)
    {
        vx_enum policy = test_policies[ipolicy];
        vx_enum rounding = test_rounding_policies[ipolicy];
        /*! \bug This is not strictly what we want for conformance */
        int isrc0_format;
        for (isrc0_format = 0; isrc0_format < (int)dimof(test_formats); isrc0_format++)
        {
            vx_df_image src0_format = test_formats[isrc0_format];
            vx_imagepatch_addressing_t image0_addr;
            int isrc1_format;

            src0_image = vx_create_image_valuecovering(context, src0_format, width, height, 130613, 1);
            if (!src0_image)
                VFAIL(release_context,
                      "can't create test-image for first arithmetic operand (%d, %d)",
                      ipolicy, isrc0_format);

            for (isrc1_format = 0; isrc1_format < (int)dimof(test_formats); isrc1_format++)
            {
                vx_df_image src1_format = test_formats[isrc1_format];
                vx_imagepatch_addressing_t image1_addr;
                int idest_format;

                /* We deliberately use periodicity 256 for both VX_DF_IMAGE_U8 and VX_DF_IMAGE_S16. */
                src1_image = vx_create_image_valuecovering(context, src1_format, width, height, 20130611, 256);
                if (!src1_image)
                    VFAIL(release_src0_image, "can't create test-image for second operand (%d, %d, %d)",
                          ipolicy, isrc0_format, isrc1_format);

                for (idest_format = 0; idest_format < (int)dimof(test_formats); idest_format++)
                {
                   vx_df_image dest_format = test_formats[idest_format];
                   int iops;

                   if (dest_format == VX_DF_IMAGE_U8 && (src0_format != VX_DF_IMAGE_U8 || src1_format != VX_DF_IMAGE_U8))
                       continue;

                   for (iops = 0; iops < (int)dimof(op_tests); iops++)
                   {
                       int iscale;
                       for (iscale = 0; iscale < op_tests[iops].n_scales; iscale++)
                       {
                           vx_float32 scale = test_scales[iscale];
                           vx_imagepatch_addressing_t dest_addr;
                           void *base0 = NULL, *base1 = NULL, *dbase = NULL;
                           vx_uint32 x, y;

                           dest_image = vxCreateImage(context, width, height, dest_format);
                           if (!dest_image)
                               VFAIL(release_src1_image, "can't create dest_image (%d, %d, %d, %d, %d, %d)",
                                     ipolicy, isrc0_format, isrc1_format, idest_format, iops, iscale);

                           /*
                            * Second part of test-fixture simplification: "multiply" is assumed if
                            * there's more than one "scale" iteration, otherwise "scale" is assumed
                            * not used.
                            */
                           if (op_tests[iops].n_scales > 1)
                               status = vxuMultiply(context, src0_image, src1_image, scale, policy, rounding, dest_image);
                           else
                               status = op_tests[iops].operate_image(context, src0_image, src1_image, policy, dest_image);
                           if (status != VX_SUCCESS)
                               VFAIL(release_dest_image,
                                     "can't apply %s (%d, %d, %d, %d, %d, %d)",
                                     op_tests[iops].name,
                                     ipolicy, isrc0_format, isrc1_format, idest_format, iops, iscale);

                           status = vxAccessImagePatch(src0_image, &rect, 0, &image0_addr, &base0, VX_READ_ONLY);
                           if (status != VX_SUCCESS)
                               VFAIL(release_dest_image,
                                     "can't create patch to test-image for first arithmetic operand (%d, %d)",
                                     ipolicy, isrc0_format);

                           status = vxAccessImagePatch(src1_image, &rect, 0, &image1_addr, &base1, VX_READ_ONLY);
                           if (status != VX_SUCCESS)
                               VFAIL(release_dest_image,
                                     "can't create patch to test-image for second operand (%d, %d, %d)",
                                     ipolicy, isrc0_format, isrc1_format);

                           status = vxAccessImagePatch(dest_image, &rect, 0, &dest_addr, &dbase, VX_READ_ONLY);
                           if (status != VX_SUCCESS)
                               VFAIL(release_dest_image,
                                     "can't create patch to dest_image (%d, %d, %d, %d, %d, %d)",
                                     ipolicy, isrc0_format, isrc1_format, idest_format, iops, iscale);

                           for (y = 0; y < height; y++)
                           {
                             for (x = 0; x < width; x++)
                             {
                               void *img0 = vxFormatImagePatchAddress2d(base0, x, y, &image0_addr);
                               void *img1 = vxFormatImagePatchAddress2d(base1, x, y, &image1_addr);
                               void *imgd = vxFormatImagePatchAddress2d(dbase, x, y, &dest_addr);
                               vx_int32 v0 = src0_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)img0 : *(vx_int16 *)img0;
                               vx_int32 v1 = src1_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)img1 : *(vx_int16 *)img1;
                               vx_int32 res = dest_format == VX_DF_IMAGE_U8 ? *(vx_uint8 *)imgd : *(vx_int16 *)imgd;
                               vx_int32 ref = op_tests[iops].refop(v0, v1, scale, dest_format, policy);

                               if (ref != res)
                                   VFAIL(release_dest_image,
                                         "wrong result at (x, y) = (%d, %d) for %s:%s(0x%x:%s, 0x%x:%s, %f);"
                                         " 0x%x:%s should be 0x%x:%s",
                                         x, y,
                                         op_tests[iops].name,
                                         policy == VX_CONVERT_POLICY_WRAP ? "WRAP" : "SAT",
                                         v0, src0_format == VX_DF_IMAGE_U8 ? "U8" : "S16",
                                         v1, src1_format == VX_DF_IMAGE_U8 ? "U8" : "S16",
                                         scale,
                                         res, dest_format == VX_DF_IMAGE_U8 ? "U8" : "S16",
                                         ref, dest_format == VX_DF_IMAGE_U8 ? "U8" : "S16");
                             }
                           }

                           vxCommitImagePatch(dest_image, NULL, 0, &dest_addr, dbase);
                           vxCommitImagePatch(src1_image, NULL, 0, &image1_addr, base1);
                           vxCommitImagePatch(src0_image, NULL, 0, &image0_addr, base0);

                           vxReleaseImage(&dest_image);
                       }
                   }
                }
                vxReleaseImage(&src1_image);
            }
            vxReleaseImage(&src0_image);
        }
    }

    status |= vxUnloadKernels(context, "openvx-debug");

    /*
     * If we got here, everything checked out. We've already released
     * the node, the graph and the images, so skip those parts.
     */
    goto release_context;

release_dest_image:
    vxReleaseImage(&dest_image);

release_src1_image:
    vxReleaseImage(&src1_image);

release_src0_image:
    vxReleaseImage(&src0_image);

release_context:

    vxReleaseContext(&context);

    return status;
}

vx_status vx_test_graph_corners(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 width = 160, height = 120, n;
        vx_float32 k = 0.15f;
        vx_float32 b = 0.47f;
        vx_float32 str = 10000.0f;
        vx_float32 min_d = 2.0f;
        vx_uint32 ws = 5, bs = 3;

        vx_scalar scalars[] = {
                vxCreateScalar(context, VX_TYPE_FLOAT32, &str),
                vxCreateScalar(context, VX_TYPE_FLOAT32, &min_d),
                vxCreateScalar(context, VX_TYPE_FLOAT32, &k),
                vxCreateScalar(context, VX_TYPE_FLOAT32, &b)
        };
        vx_image images[] = {
                vxCreateImage(context, width, height, VX_DF_IMAGE_U8),
                vxCreateImage(context, width, height, VX_DF_IMAGE_U8),
        };
        vx_array harris_arr = vxCreateArray(context, VX_TYPE_KEYPOINT, 1000);
        vx_array fast_arr = vxCreateArray(context, VX_TYPE_KEYPOINT, 1000);
        vx_graph graph = vxCreateGraph(context);
        vxLoadKernels(context, "openvx-debug");
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_node nodes[] = {
                vxFReadImageNode(graph, "shapes.pgm", images[0]),
                vxGaussian3x3Node(graph, images[0], images[1]),
                vxFWriteImageNode(graph, images[1], "oshapes_blurred.pgm"),
                vxHarrisCornersNode(graph, images[1], scalars[0], scalars[1], scalars[2], ws, bs, harris_arr, NULL),
                vxFastCornersNode(graph, images[1], scalars[3], vx_false_e, fast_arr, NULL),
            };
            CHECK_ALL_ITEMS(nodes, n, status, exit);
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                vx_size num_items = 0;
                vx_rectangle_t rect;

                status = vxProcessGraph(graph);

                vxGetValidRegionImage(images[1], &rect);
                vxQueryArray(harris_arr, VX_ARRAY_NUMITEMS, &num_items, sizeof(num_items));
                printf("There are "VX_FMT_SIZE" number of points in the harris array!\n", num_items);
                vxQueryArray(fast_arr, VX_ARRAY_NUMITEMS, &num_items, sizeof(num_items));
                printf("There are "VX_FMT_SIZE" number of points in the fast array!\n", num_items);
                printf("Rectangle from Gaussian is {%d, %d, %d, %d}\n", rect.start_x, rect.start_y, rect.end_x, rect.end_y);
            }
exit:
            for (n = 0; n < dimof(nodes); n++)
            {
                vxReleaseNode(&nodes[n]);
            }
            vxReleaseGraph(&graph);
        }
        status |= vxUnloadKernels(context, "openvx-debug");
        for (n = 0; n < dimof(images); n++)
        {
            vxReleaseImage(&images[n]);
        }
        for (n = 0; n < dimof(scalars); n++)
        {
            vxReleaseScalar(&scalars[n]);
        }
        vxReleaseArray(&harris_arr);
        vxReleaseArray(&fast_arr);
        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_test_graph_tracker(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    (void)argc;
    (void)argv;

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_uint32 n;
        vx_uint32 width = 3072;
        vx_size winSize = 32;
        vx_uint32 height = 2048;
        vx_float32  sens_thresh = 20000000000.0f;
        vx_enum criteria = VX_TERM_CRITERIA_BOTH;// lk params
        vx_float32 epsilon = 0.01f;
        vx_uint32 num_iterations = 10;
        vx_bool use_initial_estimate = vx_true_e;
        vx_float32 min_distance = 4.9f;// harris params
        vx_float32 sensitivity = 0.041f;
        vx_int32 gradient_size = 3;
        vx_int32 block_size = 3;
        vx_uint32 num_corners = 0;
        vx_array old_features = vxCreateArray(context, VX_TYPE_KEYPOINT, 2000);
        vx_array new_features = vxCreateArray(context, VX_TYPE_KEYPOINT, 2000);
        vx_scalar epsilon_s = vxCreateScalar(context,VX_TYPE_FLOAT32,&epsilon);
        vx_scalar num_iterations_s =  vxCreateScalar(context,VX_TYPE_UINT32,&num_iterations);
        vx_scalar use_initial_estimate_s =  vxCreateScalar(context,VX_TYPE_BOOL,&use_initial_estimate);
        vx_scalar min_distance_s =  vxCreateScalar(context,VX_TYPE_FLOAT32,&min_distance);
        vx_scalar sensitivity_s =  vxCreateScalar(context,VX_TYPE_FLOAT32,&sensitivity);
        vx_scalar sens_thresh_s = vxCreateScalar(context,VX_TYPE_FLOAT32,&sens_thresh);
        vx_scalar num_corners_s = vxCreateScalar(context,VX_TYPE_SIZE,&num_corners);;

        vx_scalar scalars[] = {
                epsilon_s,
                num_iterations_s,
                use_initial_estimate_s,
                min_distance_s,
                sensitivity_s,
                sens_thresh_s,
                num_corners_s
        };

        vx_array arrays[] = {
                old_features,
                new_features
        };

        vx_image images[] = {
            vxCreateImage(context, width, height, VX_DF_IMAGE_UYVY),     // index 0: Input 1
            vxCreateImage(context, width, height, VX_DF_IMAGE_UYVY),     // index 1: Input 2
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),       // index 2: Get Y channel
            vxCreateImage(context, width, height, VX_DF_IMAGE_U8),       // index 3: Get Y channel
        };
        vx_pyramid pyramid_new = vxCreatePyramid(context,4,0.5f, width, height, VX_DF_IMAGE_U8);
        vx_pyramid pyramid_old = vxCreatePyramid(context,4,0.5f, width, height, VX_DF_IMAGE_U8);

        vx_pyramid pyramids[] = {
                pyramid_new,
                pyramid_old
        };

        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_node nodes[] = {
                vxChannelExtractNode(graph, images[0], VX_CHANNEL_Y, images[2]),
                vxGaussianPyramidNode(graph, images[2], pyramid_new),
                vxChannelExtractNode(graph, images[1], VX_CHANNEL_Y, images[3]),
                vxGaussianPyramidNode(graph, images[3], pyramid_old),
                vxHarrisCornersNode(graph, images[3], sens_thresh_s, min_distance_s,sensitivity_s,gradient_size,block_size, old_features,num_corners_s),
                vxOpticalFlowPyrLKNode(graph, pyramid_old,  pyramid_new, old_features,old_features,new_features,criteria,epsilon_s,num_iterations_s,use_initial_estimate_s,winSize )
             };

            CHECK_ALL_ITEMS(nodes, n, status, exit);
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {


                status |= vxuFReadImage(context,"superresFirst_3072x2048_UYVY.yuv", images[0]);
                status |= vxuFReadImage(context,"superresSecond_3072x2048_UYVY.yuv", images[1]);

                if (status == VX_SUCCESS)
                    status = vxProcessGraph(graph);

            }
exit:
            for (n = 0; n < dimof(nodes); n++)
            {
                vxReleaseNode(&nodes[n]);
            }
            vxReleaseGraph(&graph);
         }
        for (n = 0; n < dimof(images); n++)
        {
            vxReleaseImage(&images[n]);
        }
        for (n = 0; n < dimof(pyramids); n++)
        {
            vxReleasePyramid(&pyramids[n]);
        }
        for (n = 0; n < dimof(arrays); n++)
        {
            vxReleaseArray(&arrays[n]);
        }
        for (n = 0; n < dimof(scalars); n++)
        {
            vxReleaseScalar(&scalars[n]);
        }
        vxReleaseContext(&context);
    }
    return status;
}

#if defined(EXPERIMENTAL_USE_DOT)
vx_status vx_test_graph_dot_export(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = vxCreateContext();
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_graph graph = vxCreateGraph(context);
        vxLoadKernels(context, "openvx-debug");
        status = vxGetStatus((vx_reference)graph);
        if (status == VX_SUCCESS)
        {
            vx_image images[] = {
                vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
                vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
            };
            vx_node nodes[] = {
                vxFReadImageNode(graph, "lena_512x512.pgm", images[0]),
                vxGaussian3x3Node(graph, images[0], images[1]),
                vxFWriteImageNode(graph, images[1], "oblena_512x512.pgm"),
            };
            vx_uint32 n;
            CHECK_ALL_ITEMS(nodes, n, status, exit);
            status = vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxExportGraphToDot(graph, "blur.dot", vx_false_e);
                status = vxExportGraphToDot(graph, "blur_data.dot", vx_true_e);
            }
            vxReleaseGraph(&graph);
        }
        status |= vxUnloadKernels(context, "openvx-debug");
exit:
        vxReleaseContext(&context);
    }
    return status;
}
#endif

#if defined(OPENVX_USE_XML)
vx_status vx_xml_fullimport(int argc, char *argv[])
{
    vx_status status = VX_FAILURE;
    vx_context context = 0;
    char filename[256] = "test.xml";
    context = vxCreateContext();
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_reference ref = 0;
        vx_import xml_import;
        vx_uint32 count = 0;
        vx_uint32 num_refs = 0;
        vx_uint32 num_refs_orig = 0;
        vx_uint32 num_refs_added = 0;
        vx_uint32 num_kernels = 0;
        vx_uint32 num_kernels_orig = 0;
        vx_uint32 num_kernels_added = 0;
        vx_uint32 num_nonkern_added = 0;
        vx_status import_status = VX_FAILURE;
        vxDirective((vx_reference)context, VX_DIRECTIVE_ENABLE_PERFORMANCE);
        vxQueryContext(context, VX_CONTEXT_REFERENCES, &num_refs_orig, sizeof(num_refs_orig));
        vxQueryContext(context, VX_CONTEXT_UNIQUE_KERNELS, &num_kernels_orig, sizeof(num_kernels_orig));

        xml_import = vxImportFromXML(context, filename);
        if(xml_import && (import_status = vxGetStatus((vx_reference)xml_import)) == VX_SUCCESS) {

            vxQueryContext(context, VX_CONTEXT_REFERENCES, &num_refs, sizeof(num_refs));
            vxQueryContext(context, VX_CONTEXT_UNIQUE_KERNELS, &num_kernels, sizeof(num_kernels));
            num_refs_added = num_refs - num_refs_orig;
            num_kernels_added = num_kernels - num_kernels_orig;
            num_nonkern_added = num_refs_added-num_kernels_added;

            if((status = vxQueryImport(xml_import, VX_IMPORT_ATTRIBUTE_COUNT, &count, sizeof(count))) == VX_SUCCESS) {

                /* Count returns the number of references explicitly found in the xml file and returned in the refs array.
                 * num_nonkern_added can be greater than count because of the following 2 reasons:
                 *    1. if there are any non-unique kernels added from a library, this will increase the num_refs count,
                 *       but not the count of the number of unique kernels
                 *    2. if there are any virtual pyramids which do not have image references in the xml file, there will
                 *       be added references for each level of the pyramid, but these will not be listed in the refs array
                 */
                if (num_nonkern_added < count) {
                    printf("ODD? Num nonkernel Refs Added = %u and Count = %u\n", num_nonkern_added, count);
                    status = VX_FAILURE;
                }
            }

            /* Now that the objects are imported, execute all graphs and report performance */
            if(status == VX_SUCCESS) {

                vx_uint32 i;
                vx_enum type;
                vx_status graphStatus = VX_SUCCESS;

                /* Option 1: No known names, check all in a loop */
                for(i = 0; i<count; i++) {
                    ref = vxGetImportReferenceByIndex(xml_import, i);
                    if(vxQueryReference(ref, VX_REF_ATTRIBUTE_TYPE, &type, sizeof(type)) == VX_SUCCESS) {
                        if(type == VX_TYPE_GRAPH) {
                            vx_perf_t perf;
                            graphStatus |= vxProcessGraph((vx_graph)ref);
                            vxQueryGraph((vx_graph)ref, VX_GRAPH_PERFORMANCE, &perf, sizeof(perf));
                            printf("Graph "VX_FMT_SIZE" avg time: %lu\n", (vx_size)ref, perf.avg);
                        }
                        vxReleaseReference(&ref);
                    }
                }

                if(graphStatus != VX_SUCCESS)
                    status = VX_FAILURE;

                /* Option 2.a: Use known names */
                vx_char *names[2] = { "GRAPH1", "GRAPH2"};
                vx_reference refs[2];
                for(i = 0; i<2; i++) {
                    refs[i] = vxGetImportReferenceByName(xml_import, names[i]);
                    if(refs[i]) {
                        vx_perf_t perf;
                        graphStatus |= vxProcessGraph((vx_graph)refs[i]);
                        vxQueryGraph((vx_graph)refs[i], VX_GRAPH_PERFORMANCE, &perf, sizeof(perf));
                        printf("Graph "VX_FMT_SIZE" avg time: %lu\n", (vx_size)refs[i], perf.avg);
                        vxReleaseReference(&refs[i]);
                    }
                }

                /* Option 2.b: Use known names and release import object before graphs (should work) */
                refs[0] = vxGetImportReferenceByName(xml_import, names[0]);
                refs[1] = vxGetImportReferenceByName(xml_import, names[1]);
                vxReleaseImport(&xml_import);

                for(i = 0; i<2; i++) {
                    if(refs[i]) {
                        vx_perf_t perf;
                        graphStatus |= vxProcessGraph((vx_graph)refs[i]);
                        vxQueryGraph((vx_graph)refs[i], VX_GRAPH_PERFORMANCE, &perf, sizeof(perf));
                        printf("Graph "VX_FMT_SIZE" avg time: %lu\n", (vx_size)refs[i], perf.avg);
                        vxReleaseReference(&refs[i]);
                    }
                }
            }
        }
        status |= import_status;
        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_xml_fullexport(int argc, char *argv[])
{
    vx_char xmlfile[256] = "openvx.xml";
    vx_status status = VX_FAILURE;
    vx_context context = 0;
    context = vxCreateContext();
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vxLoadKernels(context, "openvx-debug");

        vx_uint32 w = 6;
        vx_uint32 h = 4;
        vx_image images[] = {
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
            vxCreateImage(context, w, h, VX_DF_IMAGE_U8),
            vxCreateImage(context, w, h, VX_DF_IMAGE_S16),
            vxCreateImage(context, w, h, VX_DF_IMAGE_U16),
            vxCreateImage(context, w, h, VX_DF_IMAGE_S32),
            vxCreateImage(context, w, h, VX_DF_IMAGE_U32),
            vxCreateImage(context, w, h, VX_DF_IMAGE_RGB),
            vxCreateImage(context, w, h, VX_DF_IMAGE_RGBX),
            vxCreateImage(context, w, h, VX_DF_IMAGE_UYVY),
            vxCreateImage(context, w, h, VX_DF_IMAGE_YUYV),
            vxCreateImage(context, w, h, VX_DF_IMAGE_IYUV),
            vxCreateImage(context, w, h, VX_DF_IMAGE_YUV4),
            vxCreateImage(context, w, h, VX_DF_IMAGE_NV12),
            vxCreateImage(context, w, h, VX_DF_IMAGE_NV21),
        };
        vxSetReferenceName((vx_reference)images[0], "INPUT_IMG");
        vxSetReferenceName((vx_reference)images[1], "OUTPUT_IMG");
        vx_char l = 'z';
        vx_float32 pi = M_PI;
        vx_float64 tau = (vx_float64)VX_TAU;
        vx_df_image code = VX_DF_IMAGE_NV12;
        vx_uint8 v = UINT8_MAX;
        vx_int8 vi = INT8_MIN;
        vx_uint16 u16t = UINT16_MAX;
        vx_int16 grad = INT16_MIN;
        vx_uint64 u64t = UINT64_MAX;
        vx_int64 rid = INT64_MIN;
        vx_uint32 u32t = UINT32_MAX;
        vx_size len = 100;
        vx_bool bt = vx_true_e;
        vx_matrix matf = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3);
        vx_matrix mati = vxCreateMatrix(context, VX_TYPE_INT32, 3, 3);
        vx_scalar scalars[] = {
            vxCreateScalar(context, VX_TYPE_CHAR, &l),
            vxCreateScalar(context, VX_TYPE_UINT8, &v),
            vxCreateScalar(context, VX_TYPE_INT8, &vi),
            vxCreateScalar(context, VX_TYPE_FLOAT32, &pi),
            vxCreateScalar(context, VX_TYPE_FLOAT64, &tau),
            vxCreateScalar(context, VX_TYPE_DF_IMAGE, &code),
            vxCreateScalar(context, VX_TYPE_UINT16, &u16t),
            vxCreateScalar(context, VX_TYPE_INT16, &grad),
            vxCreateScalar(context, VX_TYPE_UINT64, &u64t),
            vxCreateScalar(context, VX_TYPE_INT64, &rid),
            vxCreateScalar(context, VX_TYPE_UINT32, &u32t),
            vxCreateScalar(context, VX_TYPE_SIZE, &len),
            vxCreateScalar(context, VX_TYPE_BOOL, &bt),
        };
        vx_threshold thresh = vxCreateThreshold(context, VX_THRESHOLD_TYPE_BINARY, VX_TYPE_UINT8);
        vx_threshold dblthr = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8);
        vx_convolution conv = vxCreateConvolution(context, 3, 3);
        vx_lut lut = vxCreateLUT(context, VX_TYPE_UINT8, 256);
        vx_remap remap = vxCreateRemap(context, w, h, w, h);
        vx_distribution dist = vxCreateDistribution(context, 16, 0, 256);
        vx_pyramid pyr = vxCreatePyramid(context, 4, VX_SCALE_PYRAMID_HALF, w*4, h*4, VX_DF_IMAGE_U8);
        vx_pyramid pyr2 = vxCreatePyramid(context, 4, VX_SCALE_PYRAMID_HALF, w*4, h*4, VX_DF_IMAGE_U8);
        vx_pyramid pyr3 = vxCreatePyramid(context, 4, VX_SCALE_PYRAMID_HALF, w*4, h*4, VX_DF_IMAGE_U8);
        vx_image fromPyr[] = {
            vxGetPyramidLevel(pyr, 0),
            vxGetPyramidLevel(pyr2, 0),
            vxGetPyramidLevel(pyr3, 0),
        };
        vx_graph graph = vxCreateGraph(context);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vxSetReferenceName((vx_reference)graph, "GRAPH1");
            vx_int32 th = 240, lwr = 10;
            vx_float32 fmat[3][3] = {
                    {-M_PI, 0, M_PI},
                    {-VX_TAU, 0, VX_TAU},
                    {-M_PI, 0, M_PI},
            };
            vx_int32 imat[3][3] = {
                    {-23, 904, 29},
                    {782, 823, 288},
                    {848, 828, 994},
            };
            vx_int16 sobel_x[3][3] = {
                    {-1, 0, 1},
                    {-2, 0, 1},
                    {-1, 0, 1},
            };
            vx_int32 histogram[16], *histo = &histogram[0];
            vx_uint8 mylut[256] = {0};
            vx_uint32 y, x, cscale = 16;
            vx_imagepatch_addressing_t addr = {0};
            void *base = NULL;
            vx_rectangle_t rect;
            vx_image virts[] = {
                    vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT), /* no specified dimension or format */
                    vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_S16), /* no specified dimension */
                    vxCreateVirtualImage(graph, 320, 240, VX_DF_IMAGE_VIRT), /* no specified format */
                    vxCreateVirtualImage(graph, 640, 480, VX_DF_IMAGE_U8), /* no user access */
            };
            vxCreateVirtualPyramid(graph, 4, VX_SCALE_PYRAMID_HALF, 0, 0, VX_DF_IMAGE_VIRT); /* no dimension and format specified for level 0 */
            vxCreateVirtualPyramid(graph, 4, VX_SCALE_PYRAMID_HALF, 640, 480, VX_DF_IMAGE_VIRT); /* no format specified. */
            vxCreateVirtualPyramid(graph, 4, VX_SCALE_PYRAMID_HALF, 640, 480, VX_DF_IMAGE_U8); /* no access */

            vxCreateVirtualArray(graph, 0, 0); /* totally unspecified */
            vxCreateVirtualArray(graph, VX_TYPE_KEYPOINT, 0); /* unspecified capacity */
            vxCreateVirtualArray(graph, VX_TYPE_KEYPOINT, 1000); /* no access */

            vx_enum policy = VX_CONVERT_POLICY_SATURATE;
            vx_int32 shift = 7;
            vx_scalar sshift = vxCreateScalar(context, VX_TYPE_INT32, &shift);

            vx_node sobelNode = vxSobel3x3Node(graph, images[0], virts[0], virts[1]);
            vx_node convNode = vxConvertDepthNode(graph, virts[0], images[1], policy, sshift);
            vx_node addNode = vxAddNode(graph, fromPyr[0], fromPyr[1], policy, fromPyr[2]);
            vx_bool replicate[] = { vx_true_e, vx_true_e, vx_false_e, vx_true_e};
            vxReplicateNode(graph, addNode, replicate, 4);

            vx_border_t border;
            border.mode = VX_BORDER_MODE_CONSTANT;
            border.constant_value.U8 = 5;
            vxSetNodeAttribute(sobelNode, VX_NODE_BORDER, &border, sizeof(border));

            vx_parameter parameters[] = {
                vxGetParameterByIndex(sobelNode, 0),
                vxGetParameterByIndex(convNode, 1)
            };
            vxAddParameterToGraph(graph, parameters[0]);
            vxAddParameterToGraph(graph, parameters[1]);

            vxCreateDelay(context, (vx_reference)images[0], 3);
            vxCreateDelay(context, (vx_reference)pyr, 2);
            vxCreateDelay(context, (vx_reference)lut, 2);
            vxCreateDelay(context, (vx_reference)matf, 2);
            vxCreateDelay(context, (vx_reference)conv, 2);
            vxCreateDelay(context, (vx_reference)dist, 2);
            vxCreateDelay(context, (vx_reference)thresh, 2);
            vxCreateDelay(context, (vx_reference)dblthr, 2);
            vxCreateDelay(context, (vx_reference)scalars[1], 4);
            vxCreateDelay(context, (vx_reference)remap, 2);

            vxCreateObjectArray(context, (vx_reference)images[0], 3);
            vxCreateObjectArray(context, (vx_reference)pyr, 2);
            vxCreateObjectArray(context, (vx_reference)lut, 2);
            vxCreateObjectArray(context, (vx_reference)matf, 2);
            vxCreateObjectArray(context, (vx_reference)conv, 2);
            vxCreateObjectArray(context, (vx_reference)dist, 2);
            vxCreateObjectArray(context, (vx_reference)thresh, 2);
            vxCreateObjectArray(context, (vx_reference)dblthr, 2);
            vxCreateObjectArray(context, (vx_reference)scalars[1], 4);
            vxCreateObjectArray(context, (vx_reference)remap, 2);

            vx_array array1 = vxCreateArray(context, VX_TYPE_UINT8, 10);
            vx_uint8 ary1[] = {2,3,4,5,6,7,8,9,10,11};
            vxAddArrayItems(array1, 10, &ary1, sizeof(vx_uint8));

            vx_array array2 = vxCreateArray(context, VX_TYPE_CHAR, 20);
            vx_char ary2[] = "a 13,.;^-";
            vxAddArrayItems(array2, 9, &ary2, sizeof(vx_char));

            vx_array array3 = vxCreateArray(context, VX_TYPE_ENUM, 4);
            vx_enum ary3[] = {VX_FAILURE, VX_SUCCESS, VX_THRESHOLD_TYPE_RANGE};
            vxAddArrayItems(array3, 3, &ary3, sizeof(vx_enum));

            vx_array array4 = vxCreateArray(context, VX_TYPE_DF_IMAGE, 4);
            vx_df_image ary4[] = {VX_DF_IMAGE_RGB, VX_DF_IMAGE_U8, VX_DF_IMAGE_VIRT};
            vxAddArrayItems(array4, 3, &ary4, sizeof(vx_df_image));

            vx_array array5 = vxCreateArray(context, VX_TYPE_KEYPOINT, 3);
            vx_keypoint_t ary5[] = {{0,0,2.3,6.55555,0.9059,5,3.5455 },{400,235,5.2222,1.221,0.5695,8,462.5 }};
            vxAddArrayItems(array5, 2, &ary5, sizeof(vx_keypoint_t));

            vx_array array6 = vxCreateArray(context, VX_TYPE_RECTANGLE, 5);
            vx_rectangle_t ary6[] = {{0,0,640,320},{65,32,128,362}};
            vxAddArrayItems(array6, 2, &ary6, sizeof(vx_rectangle_t));

            vx_array array7 = vxCreateArray(context, VX_TYPE_COORDINATES2D, 6);
            vx_coordinates2d_t ary7[] = {{1,2},{55,66}};
            vxAddArrayItems(array7, 2, &ary7, sizeof(vx_coordinates2d_t));

            vx_array array8 = vxCreateArray(context, VX_TYPE_COORDINATES3D, 6);
            vx_coordinates3d_t ary8[] = {{1,2,3},{55,66,77}};
            vxAddArrayItems(array8, 2, &ary8, sizeof(vx_coordinates3d_t));

            vx_array array9 = vxCreateArray(context, VX_TYPE_INT8, 8);
            vx_int8 ary9[] = {5, 0, -3, -8};
            vxAddArrayItems(array9, 4, &ary9, sizeof(vx_int8));

            vx_array array10 = vxCreateArray(context, VX_TYPE_INT16, 6);
            vx_int16 ary10[] = {200, 100, 0, -100, -200};
            vxAddArrayItems(array10, 5, &ary10, sizeof(vx_int16));

            vx_array array11 = vxCreateArray(context, VX_TYPE_INT32, 6);
            vx_int32 ary11[] = {200000, 100000, 0, -100000, -200000};
            vxAddArrayItems(array11, 5, &ary11, sizeof(vx_int32));

            vx_array array12 = vxCreateArray(context, VX_TYPE_BOOL, 3);
            vx_bool ary12[] = {vx_true_e, vx_false_e, vx_true_e};
            vxAddArrayItems(array12, 3, &ary12, sizeof(vx_bool));

            vx_array array13 = vxCreateArray(context, VX_TYPE_SIZE, 4);
            vx_size ary13[] = {8000, 24000};
            vxAddArrayItems(array13, 2, &ary13, sizeof(vx_size));

            vx_array array14 = vxCreateArray(context, VX_TYPE_FLOAT64, 2);
            vx_float64 ary14[] = {1235.25566, -563.2567};
            vxAddArrayItems(array14, 2, &ary14, sizeof(vx_float64));

            vx_array array15 = vxCreateArray(context, VX_TYPE_UINT64, 8);
            vx_uint64 ary15[] = {9000000000, 8000000000, 7000000000, 6000000000};
            vxAddArrayItems(array15, 4, &ary15, sizeof(vx_uint64));

            vx_array array16 = vxCreateArray(context, VX_TYPE_UINT16, 6);
            vx_uint16 ary16[] = {290, 100, 0, 100, 260};
            vxAddArrayItems(array16, 5, &ary16, sizeof(vx_uint16));

            vx_array array17 = vxCreateArray(context, VX_TYPE_UINT32, 6);
            vx_uint32 ary17[] = {200000, 100000, 0, 100000, 200000};
            vxAddArrayItems(array17, 5, &ary17, sizeof(vx_uint32));

            vx_array array18 = vxCreateArray(context, VX_TYPE_FLOAT32, 2);
            vx_float32 ary18[] = {1235.25566, -563.2567};
            vxAddArrayItems(array18, 2, &ary18, sizeof(vx_float32));

            vx_array array19 = vxCreateArray(context, VX_TYPE_INT64, 8);
            vx_int64 ary19[] = {9000000000, 8000000000, -7000000000, -6000000000};
            vxAddArrayItems(array19, 4, &ary19, sizeof(vx_int64));

            vx_delay delayArr = vxCreateDelay(context, (vx_reference)array7, 4);
            vx_array  ary7_3 = (vx_array)vxGetReferenceFromDelay(delayArr, -3);
            vxAddArrayItems(ary7_3, 2, &ary7, sizeof(vx_array));

            typedef struct _mystruct {
                vx_uint32 some_uint;
                vx_float64 some_double;
            } mystruct;
            vx_enum mytype = vxRegisterUserStruct(context, sizeof(mystruct));
            vx_array structArray = vxCreateArray(context, mytype, 4);
            mystruct mystructdata[2] = {{32, 3.2},{64, 6.4}};
            vxAddArrayItems(structArray, 2, &mystructdata, sizeof(mystruct));

            vxCreateDelay(context, (vx_reference)structArray, 2);

            vxGetValidRegionImage(images[0],&rect);
            for (y = 0u; y < h; y++)
            {
                for (x = 0u; x < w; x++)
                {
                    // just some equation to see variation.
                    vx_float32 sx = ((vx_float32)x/2) + 0.5f;
                    vx_float32 sy = ((vx_float32)y/2) + 0.5f;
                    if (vxSetRemapPoint(remap, x, y, sx, sy) != VX_SUCCESS) {
                        printf("Failed to set remap point %ux%u to %fx%f\n", x, y, sx, sy);
                    }
                }
            }

            vxAccessImagePatch(images[0], &rect, 0, &addr, &base, VX_WRITE_ONLY);
            for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                    vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                    *ptr = y*addr.dim_x+x;
                }
            }
            vxCommitImagePatch(images[0], &rect, 0, &addr, base);

            base = NULL;
            vxGetValidRegionImage(images[2],&rect);
            vxAccessImagePatch(images[2], &rect, 0, &addr, &base, VX_WRITE_ONLY);
            for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                    vx_int16 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                    *ptr = INT16_MIN+(y*addr.dim_x+x);
                }
            }
            vxCommitImagePatch(images[2], &rect, 0, &addr, base);

            base = NULL;
            vxGetValidRegionImage(images[3],&rect);
            vxAccessImagePatch(images[3], &rect, 0, &addr, &base, VX_WRITE_ONLY);
            for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                    vx_uint16 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                    *ptr = UINT16_MAX-(y*addr.dim_x+x);
                }
            }
            vxCommitImagePatch(images[3], &rect, 0, &addr, base);

            base = NULL;
            vxGetValidRegionImage(images[4],&rect);
            vxAccessImagePatch(images[4], &rect, 0, &addr, &base, VX_WRITE_ONLY);
            for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                    vx_int32 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                    *ptr = INT32_MIN+(y*addr.dim_x+x);
                }
            }
            vxCommitImagePatch(images[4], &rect, 0, &addr, base);

            base = NULL;
            vxGetValidRegionImage(images[5],&rect);
            vxAccessImagePatch(images[5], &rect, 0, &addr, &base, VX_WRITE_ONLY);
            for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                    vx_uint32 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                    *ptr = INT32_MAX-(y*addr.dim_x+x);
                }
            }
            vxCommitImagePatch(images[5], &rect, 0, &addr, base);

            base = NULL;
            vxGetValidRegionImage(images[6],&rect);
            vxAccessImagePatch(images[6], &rect, 0, &addr, &base, VX_WRITE_ONLY);
            for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                    vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                    ptr[0] = 0+(y*addr.dim_x+x);
                    ptr[1] = 1+(y*addr.dim_x+x);
                    ptr[2] = 2+(y*addr.dim_x+x);
                }
            }
            vxCommitImagePatch(images[6], &rect, 0, &addr, base);

            base = NULL;
            vxGetValidRegionImage(images[7],&rect);
            vxAccessImagePatch(images[7], &rect, 0, &addr, &base, VX_WRITE_ONLY);
            for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                    vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                    ptr[0] = 0+(y*addr.dim_x+x);
                    ptr[1] = 1+(y*addr.dim_x+x);
                    ptr[2] = 2+(y*addr.dim_x+x);
                    ptr[3] = 3+(y*addr.dim_x+x);
                }
            }
            vxCommitImagePatch(images[7], &rect, 0, &addr, base);

            /* 422 interleaved types*/
            for(int i=8; i<=9; i++) {
                base = NULL;
                vxGetValidRegionImage(images[i],&rect);
                vxAccessImagePatch(images[i], &rect, 0, &addr, &base, VX_WRITE_ONLY);
                for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                    for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                        vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                        ptr[0] = 0+(y*addr.dim_x+x);
                        ptr[1] = 1+(y*addr.dim_x+x);
                    }
                }
                vxCommitImagePatch(images[i], &rect, 0, &addr, base);
            }

            /* 3 plane types */
            for(int i=10; i<=11; i++) {
                for(int p=0; p<=2; p++) {
                    base = NULL;
                    vxGetValidRegionImage(images[i],&rect);
                    vxAccessImagePatch(images[i], &rect, p, &addr, &base, VX_WRITE_ONLY);
                    for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                        for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                            vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                            ptr[0] = 0+(y*addr.dim_x+x);
                        }
                    }
                    vxCommitImagePatch(images[i], &rect, p, &addr, base);
                }
            }

            /* semiplanar image types */
            for(int i=12; i<=13; i++) {
                base = NULL;
                vxGetValidRegionImage(images[i],&rect);
                vxAccessImagePatch(images[i], &rect, 0, &addr, &base, VX_WRITE_ONLY);
                for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                    for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                        vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                        *ptr = 0+(y*addr.dim_x+x);
                    }
                }
                vxCommitImagePatch(images[i], &rect, 0, &addr, base);
                base = NULL;
                vxAccessImagePatch(images[i], &rect, 1, &addr, &base, VX_WRITE_ONLY);
                //todo not sure about chroma semiplane
                for (y = 0u; y < addr.dim_y; y+=addr.step_y) {
                    for (x = 0u; x < addr.dim_x; x+=addr.step_x) {
                        vx_uint8 *ptr = vxFormatImagePatchAddress2d(base, x, y, &addr);
                        ptr[0] = 0+(y*addr.dim_x+x);
                        ptr[1] = 1+(y*addr.dim_x+x);
                    }
                }
                vxCommitImagePatch(images[i], &rect, 1, &addr, base);
            }

            vxAccessLUT(lut, (void **)&mylut, VX_READ_AND_WRITE);
            for (x = 0u; x < 256; x++)
                mylut[x] = (vx_uint8)((x-1)&0xFF);
            vxCommitLUT(lut, mylut);
            vxCopyMatrix(matf, fmat, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            vxCopyMatrix(mati, imat, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            vxSetConvolutionAttribute(conv, VX_CONVOLUTION_SCALE, &cscale, sizeof(cscale));
            vxCopyConvolutionCoefficients(conv, (vx_int16 *)sobel_x, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
            vxSetThresholdAttribute(thresh, VX_THRESHOLD_THRESHOLD_VALUE, &th, sizeof(th));
            vxSetThresholdAttribute(dblthr, VX_THRESHOLD_THRESHOLD_LOWER, &lwr, sizeof(lwr));
            vxSetThresholdAttribute(dblthr, VX_THRESHOLD_THRESHOLD_UPPER, &th, sizeof(th));
            {
                vx_map_id map_id = 0;
                vx_uint32 ptr = 0;
                vx_size nbins = 0;
                vxQueryDistribution(dist, VX_DISTRIBUTION_BINS, &nbins, sizeof(nbins));
                vxMapDistribution(dist, &map_id, (void**)&ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);
                for (x = 0; x < (vx_uint32)nbins; x++)
                {
                    vx_size h = nbins / 2;
                    if (x > (vx_uint32)h)
                        histogram[x] = (vx_uint32)(h - (x - h));
                    else
                        histogram[x] = x;
                }
                vxUnmapDistribution(dist, map_id);
            }
            rect.start_x = 2;
            rect.start_y = 1;
            rect.end_x = 4;
            rect.end_y = 2;
            vx_image roi = vxCreateImageFromROI(images[0], &rect);
            vxCreateImageFromROI(images[0], &rect);
            rect.start_x = 0;
            rect.start_y = 0;
            rect.end_x = 1;
            rect.end_y = 1;
            vxCreateImageFromROI(roi, &rect);
            vx_pixel_value_t rgbPixel = {{ 0, 255, 128 }};
            vxCreateUniformImage(context, 64, 32, VX_DF_IMAGE_RGB, &rgbPixel);
            status = vxExportToXML(context, xmlfile);
            vxReleaseGraph(&graph);
        }
        status |= vxUnloadKernels(context, "openvx-debug");

        vxReleaseContext(&context);
    }
    return status;
}

vx_status vx_xml_loopback(int argc, char *argv[])
{
    vx_char xmlfile[32] = "openvx.xml";
    vx_char xmlfile1[32] = "openvx1.xml";
    vx_status status = VX_FAILURE;
    vx_context context = 0;

    /* Call the full export test, which writes to openvx.xml */
    status = vx_xml_fullexport(argc, argv);

    context = vxCreateContext();
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_import xml_import;
        vx_status import_status = VX_FAILURE;

        /* Import that which was previously exported */
        xml_import = vxImportFromXML(context, xmlfile);

        /* Export again so we can compare first xml file to second one */
        status |= vxExportToXML(context, xmlfile1);

        if(xml_import && (import_status = vxGetStatus((vx_reference)xml_import)) == VX_SUCCESS) {
            /* Release import object (and all the imported objects) */
            vxReleaseImport(&xml_import);
        }
        status |= import_status;

        vxReleaseContext(&context);
    }
    return status;
}
#endif

/*! The array of supported unit tests */
vx_unittest unittests[] = {
    {VX_FAILURE, "Framework: Load XYZ Extension",&vx_test_framework_load_extension},
    {VX_FAILURE, "Framework: Load Kernel Node", &vx_test_framework_load_kernel_node},
    {VX_FAILURE, "Framework: Copy",             &vx_test_framework_copy},
    {VX_FAILURE, "Framework: Copy Virtual",     &vx_test_framework_copy_virtual},
    {VX_FAILURE, "Framework: Heads",            &vx_test_framework_heads},
    {VX_FAILURE, "Framework: Unvisited",        &vx_test_framework_unvisited},
    {VX_FAILURE, "Framework: Virtual Image",    &vx_test_framework_virtualimage},
    {VX_FAILURE, "Framework: Delay",            &vx_test_framework_delay_graph},
    {VX_FAILURE, "Framework: Kernels",          &vx_test_framework_kernels},
    {VX_FAILURE, "Direct: Copy Image",          &vx_test_direct_copy_image},
    {VX_FAILURE, "Direct: Copy External Image", &vx_test_direct_copy_external_image},
    // graphs
    {VX_FAILURE, "Graph: ColorBars YUV",        &vx_test_graph_channels_yuv},
    {VX_FAILURE, "Graph: ColorBars RGB",        &vx_test_graph_channels_rgb},
    {VX_FAILURE, "Graph: bikegray",             &vx_test_graph_bikegray},
    {VX_FAILURE, "Graph: Lena",                 &vx_test_graph_lena},
    {VX_FAILURE, "Graph: Accumulates",          &vx_test_graph_accum},
    {VX_FAILURE, "Graph: Bitwise",              &vx_test_graph_bitwise},
    {VX_FAILURE, "Graph: Arithmetic",           &vx_test_graph_arit},
    {VX_FAILURE, "Graph: Corners",              &vx_test_graph_corners},
    {VX_FAILURE, "Graph: Tracker",              &vx_test_graph_tracker},
    // exports
#if defined(EXPERIMENTAL_USE_DOT)
    {VX_FAILURE, "Export: DOT",                 &vx_test_graph_dot_export},
#endif
#if defined(OPENVX_USE_XML)
    {VX_FAILURE, "Import: XML",                 &vx_xml_fullimport},
    {VX_FAILURE, "Export: XML",                 &vx_xml_fullexport},
    {VX_FAILURE, "Loopback: XML",               &vx_xml_loopback},
#endif
};

/*! \brief The main unit test.
 * \param argc The number of arguments.
 * \param argv The array of arguments.
 * \return vx_status
 * \retval 0 Success.
 * \retval !0 Failure of some sort.
 */
int main(int argc, char *argv[])
{
    vx_uint32 i;
    vx_uint32 passed = 0;
    vx_bool stopOnErrors = vx_false_e;

    if (argc == 2 && ((strncmp(argv[1], "-?", 2) == 0) ||
                      (strncmp(argv[1], "--list", 6) == 0) ||
                      (strncmp(argv[1], "-l", 2) == 0) ||
                      (strncmp(argv[1], "/?", 2) == 0)))
    {
        vx_uint32 t = 0;
        for (t = 0; t < dimof(unittests); t++)
        {
            printf("%u: %s\n", t, unittests[t].name);
        }
        /* we just want to know which graph is which */
        return 0;
    }
    else if (argc == 3 && strncmp(argv[1],"-t",2) == 0)
    {
        int c = atoi(argv[2]);
        if (c < (int)dimof(unittests))
        {
            unittests[c].status = unittests[c].unittest(argc, argv);
            printf("[%u][%s] %s, error = %d\n", c, (unittests[c].status == VX_SUCCESS?"PASSED":"FAILED"), unittests[c].name, unittests[c].status);
            return unittests[c].status;
        }
        else
            return -1;
    }
    else if (argc == 2 && strncmp(argv[1],"-s",2) == 0)
    {
        stopOnErrors = vx_true_e;
    }
    for (i = 0; i < dimof(unittests); i++)
    {
        unittests[i].status = unittests[i].unittest(argc, argv);
        if (unittests[i].status == VX_SUCCESS)
        {
            printf("[PASSED][%02u] %s\n", i, unittests[i].name);
            passed++;
        }
        else
        {
            printf("[FAILED][%02u] %s, error = %d\n", i, unittests[i].name, unittests[i].status);
            if (stopOnErrors == vx_true_e)
            {
                break;
            }
        }
    }
    printf("Passed %u out of "VX_FMT_SIZE"\n", passed, dimof(unittests));
    if (passed == dimof(unittests))
        return 0;
    else
        return -1;
}

