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
 * \brief The C-Model Target Interface
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_debug.h>
#include "vx_internal.h"
#include <vx_interface.h>
#include <vx_support.h>
#include <sys/time.h>

static const vx_char name[VX_MAX_TARGET_NAME] = "pc.opencl";

/*! \brief Prototype for assigning to kernel */
static vx_status VX_CALLBACK vxclCallOpenCLKernel(vx_node node, const vx_reference *parameters, vx_uint32 num);

static vx_cl_kernel_description_t *cl_kernels[] =
{
    &box3x3_clkernel,
	&and_kernel,
	&xor_kernel,
	&orr_kernel,
	&not_kernel,
    &gaussian3x3_clkernel,
    &sobel3x3_clkernel,
    &erode3x3_kernel,
    &dilate3x3_kernel,
    &median3x3_kernel,
	&nonlinearfilter_kernel,
    &phase_kernel,
    &absdiff_kernel,
    &threshold_clkernel,
	&warp_affine_kernel,
	&warp_perspective_kernel,
    &convolution_kernel,
};

static vx_uint32 num_cl_kernels = dimof(cl_kernels);

static void VX_CALLBACK vxcl_platform_notifier(const char *errinfo,
                                const void *private_info,
                                size_t cb,
                                void *user_data)
{
    //vx_target target = (vx_target)user_data;
    VX_PRINT(VX_ZONE_ERROR, "%s\n", errinfo);
}

vx_status vxTargetInit(vx_target_t *target)
{
    vx_status status = VX_ERROR_NO_RESOURCES;
    cl_int err = 0;
    vx_context context = target->base.context;
    cl_uint p, d, k;
    char *vx_incs = getenv("VX_CL_INCLUDE_DIR");
    //char *vx_incs = "/usr/include -I/home/pi/sample-impl-opencl/include -I/home/pi/sample-impl-opencl/include/VX";
	char *cl_dirs = getenv("VX_CL_SOURCE_DIR");
    //char *cl_dirs = "/home/pi/sample-impl-opencl/kernels/opencl";
	char cl_args[1024];

    if(NULL == vx_incs)
        return VX_FAILURE;

    snprintf(cl_args, sizeof(cl_args), "-D VX_CL_KERNEL -I %s -I %s %s %s", vx_incs, cl_dirs,
#if !defined(__APPLE__)
        "-D CL_USE_LUMINANCE",
#else
        "",
#endif
#if defined(VX_INCLUDE_DIR)
    "-I "VX_INCLUDE_DIR" "
#else
    " "
#endif
    );
    printf("flags: %s\n", cl_args);
    if (cl_dirs == NULL) {
#ifdef VX_CL_SOURCE_DIR
        const char *sdir = VX_CL_SOURCE_DIR;
        int len = strlen(sdir);
        cl_dirs = malloc(len);
        strncpy(cl_dirs, sdir, len);
#else
        return status;
#endif
    }

    strncpy(target->name, name, VX_MAX_TARGET_NAME);
    target->priority = VX_TARGET_PRIORITY_OPENCL;

    context->num_platforms = CL_MAX_PLATFORMS;
    err = clGetPlatformIDs(CL_MAX_PLATFORMS, context->platforms, NULL);
    if (err != CL_SUCCESS)
        goto exit;

    for (p = 0; p < context->num_platforms; p++) {
        err = clGetDeviceIDs(context->platforms[p], CL_DEVICE_TYPE_ALL,
            0, NULL, &context->num_devices[p]);
        err = clGetDeviceIDs(context->platforms[p], CL_DEVICE_TYPE_ALL,
            context->num_devices[p] > CL_MAX_DEVICES ? CL_MAX_DEVICES : context->num_devices[p],
            context->devices[p], NULL);
        if (err == CL_SUCCESS) {
            cl_context_properties props[] = {
                (cl_context_properties)CL_CONTEXT_PLATFORM,
                (cl_context_properties)context->platforms[p],
                (cl_context_properties)0,
            };
            for (d = 0; d < context->num_devices[p]; d++) {
                char deviceName[64];
                cl_bool compiler = CL_FALSE;
                cl_bool available = CL_FALSE;
                cl_bool image_support = CL_FALSE;
                err = clGetDeviceInfo(context->devices[p][d], CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL);
                CL_ERROR_MSG(err, "clGetDeviceInfo");
                err = clGetDeviceInfo(context->devices[p][d], CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool), &compiler, NULL);
                CL_ERROR_MSG(err, "clGetDeviceInfo");
                err = clGetDeviceInfo(context->devices[p][d], CL_DEVICE_AVAILABLE, sizeof(cl_bool), &available, NULL);
                CL_ERROR_MSG(err, "clGetDeviceInfo");
                err = clGetDeviceInfo(context->devices[p][d], CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &image_support, NULL);
                CL_ERROR_MSG(err, "clGetDeviceInfo");
                VX_PRINT(VX_ZONE_INFO, "Device %s (compiler=%s) (available=%s) (images=%s)\n", deviceName, (compiler?"TRUE":"FALSE"), (available?"TRUE":"FALSE"), (image_support?"TRUE":"FALSE"));
            }
            context->global[p] = clCreateContext(props,
                                                 context->num_devices[p],
                                                 context->devices[p],
                                                 vxcl_platform_notifier,
                                                 target,
                                                 &err);
            if (err != CL_SUCCESS)
                break;

            /* check for supported formats */
            if (err == CL_SUCCESS) {
                cl_uint f,num_entries = 0u;
                cl_image_format *formats = NULL;
                cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR;
                cl_mem_object_type type = CL_MEM_OBJECT_IMAGE2D;

                err = clGetSupportedImageFormats(context->global[p], flags, type, 0, NULL, &num_entries);
                formats = (cl_image_format *)malloc(num_entries * sizeof(cl_image_format));
                err = clGetSupportedImageFormats(context->global[p], flags, type, num_entries, formats, NULL);
                for (f = 0; f < num_entries; f++) {
                    char order[256];
                    char datat[256];
    #define CASE_STRINGERIZE2(value, string) case value: strcpy(string, #value); break
                    switch(formats[f].image_channel_order) {
                        CASE_STRINGERIZE2(CL_R, order);
                        CASE_STRINGERIZE2(CL_A, order);
                        CASE_STRINGERIZE2(CL_RG, order);
                        CASE_STRINGERIZE2(CL_RA, order);
                        CASE_STRINGERIZE2(CL_RGB, order);
                        CASE_STRINGERIZE2(CL_RGBA, order);
                        CASE_STRINGERIZE2(CL_BGRA, order);
                        CASE_STRINGERIZE2(CL_ARGB, order);
                        CASE_STRINGERIZE2(CL_INTENSITY, order);
                        CASE_STRINGERIZE2(CL_LUMINANCE, order);
                        CASE_STRINGERIZE2(CL_Rx, order);
                        CASE_STRINGERIZE2(CL_RGx, order);
                        CASE_STRINGERIZE2(CL_RGBx, order);
    #if defined(CL_VERSION_1_2) && defined(cl_khr_gl_depth_images)
                        CASE_STRINGERIZE2(CL_DEPTH, order);
                        CASE_STRINGERIZE2(CL_DEPTH_STENCIL, order);
    #if defined(__APPLE__)
                        CASE_STRINGERIZE2(CL_1RGB_APPLE, order);
                        CASE_STRINGERIZE2(CL_BGR1_APPLE, order);
                        CASE_STRINGERIZE2(CL_SFIXED14_APPLE, order);
                        CASE_STRINGERIZE2(CL_BIASED_HALF_APPLE, order);
                        CASE_STRINGERIZE2(CL_YCbYCr_APPLE, order);
                        CASE_STRINGERIZE2(CL_CbYCrY_APPLE, order);
                        CASE_STRINGERIZE2(CL_ABGR_APPLE, order);
    #endif
    #endif
                        default:
                            sprintf(order, "%x", formats[f].image_channel_order);
                            break;
                    }
                    switch(formats[f].image_channel_data_type) {
                        CASE_STRINGERIZE2(CL_SNORM_INT8, datat);
                        CASE_STRINGERIZE2(CL_SNORM_INT16, datat);
                        CASE_STRINGERIZE2(CL_UNORM_INT8, datat);
                        CASE_STRINGERIZE2(CL_UNORM_INT16, datat);
                        CASE_STRINGERIZE2(CL_UNORM_SHORT_565, datat);
                        CASE_STRINGERIZE2(CL_UNORM_SHORT_555, datat);
                        CASE_STRINGERIZE2(CL_UNORM_INT_101010, datat);
                        CASE_STRINGERIZE2(CL_SIGNED_INT8, datat);
                        CASE_STRINGERIZE2(CL_SIGNED_INT16, datat);
                        CASE_STRINGERIZE2(CL_SIGNED_INT32, datat);
                        CASE_STRINGERIZE2(CL_UNSIGNED_INT8, datat);
                        CASE_STRINGERIZE2(CL_UNSIGNED_INT16, datat);
                        CASE_STRINGERIZE2(CL_UNSIGNED_INT32, datat);
                        CASE_STRINGERIZE2(CL_HALF_FLOAT, datat);
                        CASE_STRINGERIZE2(CL_FLOAT, datat);
    #if defined(CL_VERSION_2_0)
                        CASE_STRINGERIZE2(CL_UNORM_INT24, datat);
    #endif
                        default:
                            sprintf(order, "%x", formats[f].image_channel_data_type);
                            break;
                    }
                    VX_PRINT(VX_ZONE_INFO, "%s : %s\n", order, datat);
                }
            }

            /* create a queue for each device */
            for (d = 0; d < context->num_devices[p]; d++)
            {
                context->queues[p][d] = clCreateCommandQueue(context->global[p],
                                                          context->devices[p][d],
                                                          CL_QUEUE_PROFILING_ENABLE,
                                                          &err);
                if (err == CL_SUCCESS) {
                }
            }

            /* for each kernel */
            for (k = 0; k < num_cl_kernels; k++)
            {
                char *sources = NULL;
                size_t programSze = 0;

                /* load the source file */
                VX_PRINT(VX_ZONE_INFO, "Joiner: %s\n", FILE_JOINER);
                VX_PRINT(VX_ZONE_INFO, "Path: %s\n", VX_CL_SOURCEPATH);
                VX_PRINT(VX_ZONE_INFO, "Kernel[%u] File: %s\n", k, cl_kernels[k]->sourcepath);
                VX_PRINT(VX_ZONE_INFO, "Kernel[%u] Name: %s\n", k, cl_kernels[k]->kernelname);
                VX_PRINT(VX_ZONE_INFO, "Kernel[%u] ID: %s\n", k, cl_kernels[k]->description.name);
                sources = clLoadSources(cl_kernels[k]->sourcepath, &programSze);
                /* create a program with this source */
                cl_kernels[k]->program[p] = clCreateProgramWithSource(context->global[p],
                    1,
                    (const char **)&sources,
                    &programSze,
                    &err);
                if (err == CL_SUCCESS)
                {
                    err = clBuildProgram((cl_program)cl_kernels[k]->program[p],
                        1,
                        (const cl_device_id *)context->devices,
                        (const char *)cl_args,
                        NULL,
                        NULL);
                    if (err != CL_SUCCESS)
                    {
                        CL_BUILD_MSG(err, "Build Error");
                        if (err == CL_BUILD_PROGRAM_FAILURE)
                        {
                            char log[10][1024];
                            size_t logSize = 0;
                            clGetProgramBuildInfo((cl_program)cl_kernels[k]->program[p],
                                (cl_device_id)context->devices[p][0],
                                CL_PROGRAM_BUILD_LOG,
                                sizeof(log),
                                log,
                                &logSize);
                            printf("%s\n", log);
                            VX_PRINT(VX_ZONE_ERROR, "%s", log);
                        }
                    }
                    else
                    {
                        cl_int k2 = 0;
                        cl_build_status bstatus = 0;
                        size_t bs = 0;
                        err = clGetProgramBuildInfo(cl_kernels[k]->program[p],
                            context->devices[p][0],
                            CL_PROGRAM_BUILD_STATUS,
                            sizeof(cl_build_status),
                            &bstatus,
                            &bs);
                        VX_PRINT(VX_ZONE_INFO, "Status = %d (%d)\n", bstatus, err);
                        /* get the cl_kernels from the program */
                        cl_kernels[k]->num_kernels[p] = 1;
                        err = clCreateKernelsInProgram(cl_kernels[k]->program[p],
                            1,
                            &cl_kernels[k]->kernels[p],
                            NULL);
                        VX_PRINT(VX_ZONE_INFO, "Found %u cl_kernels in %s (%d)\n", cl_kernels[k]->num_kernels[p], cl_kernels[k]->sourcepath, err);
                        for (k2 = 0; (err == CL_SUCCESS) && (k2 < (cl_int)cl_kernels[k]->num_kernels[p]); k2++)
                        {
                            char kName[VX_MAX_KERNEL_NAME];
                            size_t size = 0;
                            err = clGetKernelInfo(cl_kernels[k]->kernels[p],
                                CL_KERNEL_FUNCTION_NAME,
                                0,
                                NULL,
                                &size);
                            err = clGetKernelInfo(cl_kernels[k]->kernels[p],
                                CL_KERNEL_FUNCTION_NAME,
                                size,
                                kName,
                                NULL);
                            VX_PRINT(VX_ZONE_INFO, "Kernel %s\n", kName);
                            if (strncmp(kName, cl_kernels[k]->kernelname, VX_MAX_KERNEL_NAME) == 0)
                            {
                                vx_kernel_f kfunc = cl_kernels[k]->description.function;
                                VX_PRINT(VX_ZONE_INFO, "Linked Kernel %s on target %s\n", cl_kernels[k]->kernelname, target->name);
                                target->num_kernels++;
                                target->base.context->num_kernels++;
                                status = ownInitializeKernel(target->base.context,
                                    &target->kernels[k],
                                    cl_kernels[k]->description.enumeration,
                                    (kfunc == NULL ? vxclCallOpenCLKernel : kfunc),
                                    cl_kernels[k]->description.name,
                                    cl_kernels[k]->description.parameters,
                                    cl_kernels[k]->description.numParams,
                                    cl_kernels[k]->description.validate,
                                    cl_kernels[k]->description.input_validate,
                                    cl_kernels[k]->description.output_validate,
                                    cl_kernels[k]->description.initialize,
                                    cl_kernels[k]->description.deinitialize);
                                if (ownIsKernelUnique(&target->kernels[k]) == vx_true_e) {
                                    target->base.context->num_unique_kernels++;
                                } else {
                                    VX_PRINT(VX_ZONE_KERNEL, "Kernel %s is NOT unqiue\n", target->kernels[k].name);
                                }
                            }
                        }
                    }
                }
                else
                {
                    CL_ERROR_MSG(err, "Program");
                }
                free(sources);
            }
        }
    }
exit:
    if (err == CL_SUCCESS) {
        status = VX_SUCCESS;
    } else {
        status = VX_ERROR_NO_RESOURCES;
    }
    return status;
}

vx_status vxTargetDeinit(vx_target_t *target)
{
    vx_context context = target->base.context;
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        cl_uint p = 0, d = 0;
        vx_uint32 k = 0;
        for (p = 0; p < context->num_platforms; p++)
        {
            for (k = 0; k < num_cl_kernels; k++)
            {
                ownDecrementReference(&target->kernels[k].base, VX_INTERNAL);
                clReleaseKernel(cl_kernels[k]->kernels[p]);
                clReleaseProgram(cl_kernels[k]->program[p]);

            }
            for (d = 0; d < context->num_devices[p]; d++)
            {
                clReleaseCommandQueue(context->queues[p][d]);
            }
            clReleaseContext(context->global[p]);
        }
    }
    return VX_SUCCESS;
}

vx_status vxTargetSupports(vx_target_t *target,
                           vx_char targetName[VX_MAX_TARGET_NAME],
                           vx_char kernelName[VX_MAX_KERNEL_NAME],
#if defined(EXPERIMENTAL_USE_VARIANTS)
                           vx_char variantName[VX_MAX_VARIANT_NAME],
#endif
                           vx_uint32 *pIndex)
{
    vx_status status = VX_ERROR_NOT_SUPPORTED;
    if (strncmp(targetName, name, VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "default", VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "performance", VX_MAX_TARGET_NAME) == 0)
    {
        vx_uint32 k = 0u;
        for (k = 0u; k < VX_INT_MAX_KERNELS; k++)
        {
            if (strncmp(kernelName, target->kernels[k].name, VX_MAX_KERNEL_NAME) == 0)
            {
                status = VX_SUCCESS;
                if (pIndex) *pIndex = k;
                break;
            }
        }
    }
    return status;
}

vx_action vxTargetProcess(vx_target_t *target, vx_node_t *nodes[], vx_size startIndex, vx_size numNodes)
{
    vx_action action = VX_ACTION_CONTINUE;
    vx_status status = VX_SUCCESS;
    vx_size n = 0;
    for (n = startIndex; (n < (startIndex + numNodes)) && (action == VX_ACTION_CONTINUE); n++)
    {
        VX_PRINT(VX_ZONE_GRAPH,"Executing Kernel %s:%d in Nodes[%u] on target %s\n",
            nodes[n]->kernel->name,
            nodes[n]->kernel->enumeration,
            n,
            nodes[n]->base.context->targets[nodes[n]->affinity].name);

        ownStartCapture(&nodes[n]->perf);
        status = nodes[n]->kernel->function((vx_node)nodes[n],
                                            (vx_reference *)nodes[n]->parameters,
                                            nodes[n]->kernel->signature.num_parameters);
        nodes[n]->executed = vx_true_e;
        nodes[n]->status = status;
        ownStopCapture(&nodes[n]->perf);

        VX_PRINT(VX_ZONE_GRAPH,"kernel %s returned %d\n", nodes[n]->kernel->name, status);

        if (status == VX_SUCCESS)
        {
            /* call the callback if it is attached */
            if (nodes[n]->callback)
            {
                action = nodes[n]->callback((vx_node)nodes[n]);
                VX_PRINT(VX_ZONE_GRAPH,"callback returned action %d\n", action);
            }
        }
        else
        {
            action = VX_ACTION_ABANDON;
            VX_PRINT(VX_ZONE_ERROR, "Abandoning Graph due to error (%d)!\n", status);
        }
    }
    return action;
}

vx_status vxTargetVerify(vx_target_t *target, vx_node_t *node)
{
    vx_status status = VX_SUCCESS;
    return status;
}

vx_kernel vxTargetAddKernel(vx_target_t *target,
                            vx_char name[VX_MAX_KERNEL_NAME],
                            vx_enum enumeration,
                            vx_kernel_f func_ptr,
                            vx_uint32 numParams,
                            vx_kernel_validate_f validate,
                            vx_kernel_input_validate_f input,
                            vx_kernel_output_validate_f output,
                            vx_kernel_initialize_f initialize,
                            vx_kernel_deinitialize_f deinitialize)
{
    vx_uint32 k = 0u;
    vx_kernel_t *kernel = NULL;
    for (k = 0; k < VX_INT_MAX_KERNELS; k++)
    {
        kernel = &(target->kernels[k]);
        if (kernel->enabled == vx_false_e)
        {
            ownInitializeKernel(target->base.context,
                               kernel,
                               enumeration, func_ptr, name,
                               NULL, numParams,
                               validate, input, output, initialize, deinitialize);
            VX_PRINT(VX_ZONE_KERNEL, "Reserving %s Kernel[%u] for %s\n", target->name, k, kernel->name);
            target->num_kernels++;
            break;
        }
        kernel = NULL;
    }
    return (vx_kernel)kernel;
}

vx_cl_kernel_description_t *vxclFindKernel(vx_enum enumeration)
{
    vx_cl_kernel_description_t *vxclk = NULL;
    vx_uint32 k;
    for (k = 0; k < num_cl_kernels; k++)
    {
        if (enumeration == cl_kernels[k]->description.enumeration)
        {
            vxclk = cl_kernels[k];
            break;
        }
    }
    return vxclk;
}

/*! \brief Calls an OpenCL kernel from OpenVX Graph.
 * Steps:
 * \arg Find the target
 * \arg Get the vxcl context
 * \arg Find the kernel (to get cl kernel information)
 * \arg for each input parameter that is an object, enqueue write
 * \arg wait for finish
 * \arg for each parameter, SetKernelArg
 * \arg call kernel
 * \arg wait for finish
 * \arg for each output parameter that is an object, enqueue read
 * \arg wait for finish
 * \note This implementation will attempt to use the External API as much as possible,
 * but will cast to internal representation when needed (due to lack of API or
 * need for secret information). This is not an optimal OpenCL invocation.
 */
static vx_status VX_CALLBACK vxclCallOpenCLKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    static struct timeval start, start1, end;
    gettimeofday(&start, NULL);

    vx_status status = VX_FAILURE;
    vx_context context = node->base.context;
    vx_target target = (vx_target_t *)&node->base.context->targets[node->affinity];
    vx_cl_kernel_description_t *vxclk = vxclFindKernel(node->kernel->enumeration);
    vx_uint32 pidx, pln, didx, plidx, argidx;
    cl_int err = 0;
    size_t off_dim[3] = {0,0,0};
    size_t work_dim[3];
    //size_t local_dim[3];
    cl_event writeEvents[VX_INT_MAX_PARAMS];
    cl_event readEvents[VX_INT_MAX_PARAMS];
    cl_int we = 0, re = 0;

    // determine which platform to use
    plidx = 0;

    // determine which device to use
    didx = 0;

    /* for each input/bi data object, enqueue it and set the kernel parameters */
    for (argidx = 0, pidx = 0; pidx < num; pidx++)
    {
        vx_reference ref = node->parameters[pidx];
        vx_enum dir = node->kernel->signature.directions[pidx];
        vx_enum type = node->kernel->signature.types[pidx];
        vx_memory_t *memory = NULL;

        switch (type)
        {
            case VX_TYPE_ARRAY:
                memory = &((vx_array)ref)->memory;
                break;
            case VX_TYPE_CONVOLUTION:
                memory = &((vx_convolution)ref)->base.memory;
                break;
            case VX_TYPE_DISTRIBUTION:
                memory = &((vx_distribution)ref)->memory;
                break;
            case VX_TYPE_IMAGE:
                memory = &((vx_image)ref)->memory;
                break;
            case VX_TYPE_LUT:
                memory = &((vx_lut_t*)ref)->memory;
                break;
            case VX_TYPE_MATRIX:
                memory = &((vx_matrix)ref)->memory;
                break;
            //case VX_TYPE_PYRAMID:
            //    break;
            case VX_TYPE_REMAP:
                memory = &((vx_remap)ref)->memory;
                break;
            //case VX_TYPE_SCALAR:
            //case VX_TYPE_THRESHOLD:
            //    break;
        }
        if (memory) {
            for (pln = 0; pln < memory->nptrs; pln++) {
                if (memory->cl_type == CL_MEM_OBJECT_BUFFER) {
                    if (type == VX_TYPE_IMAGE) {

                        /* set the work dimensions */
                        work_dim[0] = memory->dims[pln][VX_DIM_X];
                        work_dim[1] = memory->dims[pln][VX_DIM_Y];

                        // width, height, stride_x, stride_y
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_int32), &memory->dims[pln][VX_DIM_X]);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_int32), &memory->dims[pln][VX_DIM_Y]);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_X]);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_Y]);
                        VX_PRINT(VX_ZONE_INFO, "Setting vx_image as Buffer with 4 parameters\n");
                    } else if (type == VX_TYPE_ARRAY || type == VX_TYPE_LUT) {
                        vx_array arr = (vx_array)ref;
                        // sizeof item, active count, capacity
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&arr->item_size);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&arr->num_items); // this is output?
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&arr->capacity);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_int32), &arr->memory.strides[VX_DIM_X]);
                        VX_PRINT(VX_ZONE_INFO, "Setting vx_buffer as Buffer with 4 parameters\n");
                    } else if (type == VX_TYPE_MATRIX) {
                        vx_matrix mat = (vx_matrix)ref;
                        // columns, rows
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&mat->columns);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&mat->rows);
                        VX_PRINT(VX_ZONE_INFO, "Setting vx_matrix as Buffer with 2 parameters\n");
                    } else if (type == VX_TYPE_DISTRIBUTION) {
                        vx_distribution dist = (vx_distribution)ref;
                        // num, range, offset, num_bins
                        vx_uint32 num_bins = dist->memory.dims[0][VX_DIM_X];
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&dist->memory.dims[VX_DIM_X]);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&dist->range_x);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&dist->offset_x);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&num_bins);
                    } else if (type == VX_TYPE_CONVOLUTION) {
                        vx_convolution conv = (vx_convolution)ref;
                        // columns, rows, scale
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&conv->base.columns);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&conv->base.rows);
                        err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint32), (vx_uint32 *)&conv->scale);
                    }
                    err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(cl_mem), &memory->hdls[pln]);
                    CL_ERROR_MSG(err, "clSetKernelArg");
                    if (dir == VX_INPUT || dir == VX_BIDIRECTIONAL)
                    {
                        gettimeofday(&start1, NULL);
                        err = clEnqueueWriteBuffer(context->queues[plidx][didx],
                                                   memory->hdls[pln],
                                                   CL_TRUE,
                                                   0,
                                                   ownComputeMemorySize(memory, pln),
                                                   memory->ptrs[pln],
                                                   0,
                                                   NULL,
                                                   &ref->event);
                        gettimeofday(&end, NULL);

                        double costTime = ((double)end.tv_sec * 1000.0 + (double)end.tv_usec / 1000.0)
                                            - ((double)start1.tv_sec * 1000.0 + (double)start1.tv_usec / 1000.0);

                        printf("opencl write DMA %f ms\n", costTime);
                    }
                } else if (memory->cl_type == CL_MEM_OBJECT_IMAGE2D) {
                    vx_rectangle_t rect = {0};
                    vx_image image = (vx_image)ref;
                    vxGetValidRegionImage(image, &rect);
                    size_t origin[3] = {rect.start_x, rect.start_y, 0};
                    size_t region[3] = {rect.end_x-rect.start_x, rect.end_y-rect.start_y, 1};
                    /* set the work dimensions */
                    work_dim[0] = rect.end_x-rect.start_x;
                    work_dim[1] = rect.end_y-rect.start_y;
                    VX_PRINT(VX_ZONE_INFO, "Setting vx_image as image2d_t wd={%zu,%zu} arg:%u\n",work_dim[0], work_dim[1], argidx);
                    err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(cl_mem), &memory->hdls[pln]);
                    CL_ERROR_MSG(err, "clSetKernelArg");
                    if (err != CL_SUCCESS) {
                        VX_PRINT(VX_ZONE_ERROR, "Error Calling Kernel %s, parameter %u\n", node->kernel->name, pidx);
                    }
                    if (dir == VX_INPUT || dir == VX_BIDIRECTIONAL)
                    {
                        err = clEnqueueWriteImage(context->queues[plidx][didx],
                                                  memory->hdls[pln],
                                                  CL_TRUE,
                                                  origin, region,
                                                  memory->strides[pln][VX_DIM_Y],
                                                  0,
                                                  memory->ptrs[pln],
                                                  0, NULL,
                                                  NULL);
                        CL_ERROR_MSG(err, "clEnqueueWriteImage");
                    }
                }
            }
        } else {
            if (type == VX_TYPE_SCALAR) {
                vx_value_t value; // largest platform atomic
                vx_size size = 0ul;
                vx_scalar sc = (vx_scalar)ref;
                vx_enum stype = VX_TYPE_INVALID;
                vxCopyScalar(sc, &value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                vxQueryScalar(sc, VX_SCALAR_TYPE, &stype, sizeof(stype));
                size = ownSizeOfType(stype);
                err = clSetKernelArg(vxclk->kernels[plidx], argidx++, size, &value);
            }
            else if (type == VX_TYPE_THRESHOLD) {
                vx_enum ttype = 0;
                vx_threshold th = (vx_threshold)ref;
                vxQueryThreshold(th, VX_THRESHOLD_TYPE, &ttype, sizeof(ttype));
                if (ttype == VX_THRESHOLD_TYPE_BINARY) {
                    err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint8), &th->value);
                } else if (ttype == VX_THRESHOLD_TYPE_RANGE) {
                    err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint8), &th->lower);
                    err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_uint8), &th->upper);
                }
            }
        }
    }
    we = 0;
    for (pidx = 0; pidx < num; pidx++) {
        vx_reference ref = node->parameters[pidx];
        vx_enum dir = node->kernel->signature.directions[pidx];
        if (dir == VX_INPUT || dir == VX_BIDIRECTIONAL) {
            memcpy(&writeEvents[we++],&ref->event, sizeof(cl_event));
        }
    }
    //local_dim[0] = 1;
    //local_dim[1] = 1;
    err = clEnqueueNDRangeKernel(context->queues[plidx][didx],
                                 vxclk->kernels[plidx],
                                 2,
                                 off_dim,
                                 work_dim,
                                 NULL,
                                 we, writeEvents, &node->base.event);

    CL_ERROR_MSG(err, "clEnqueueNDRangeKernel");
    /* enqueue a read on all output data */
    for (pidx = 0; pidx < num; pidx++)
    {
        vx_reference ref = node->parameters[pidx];
        vx_enum dir = node->kernel->signature.directions[pidx];
        vx_enum type = node->kernel->signature.types[pidx];

        if (dir == VX_OUTPUT || dir == VX_BIDIRECTIONAL)
        {
            vx_memory_t *memory = NULL;

            switch (type)
            {
                case VX_TYPE_ARRAY:
                    memory = &((vx_array)ref)->memory;
                    break;
                case VX_TYPE_CONVOLUTION:
                    memory = &((vx_convolution)ref)->base.memory;
                    break;
                case VX_TYPE_DISTRIBUTION:
                    memory = &((vx_distribution)ref)->memory;
                    break;
                case VX_TYPE_IMAGE:
                    memory = &((vx_image)ref)->memory;
                    break;
                case VX_TYPE_LUT:
                    memory = &((vx_lut_t*)ref)->memory;
                    break;
                case VX_TYPE_MATRIX:
                    memory = &((vx_matrix)ref)->memory;
                    break;
                //case VX_TYPE_PYRAMID:
                //    break;
                case VX_TYPE_REMAP:
                    memory = &((vx_remap)ref)->memory;
                    break;
                //case VX_TYPE_SCALAR:
                //case VX_TYPE_THRESHOLD:
                //    break;
            }
            if (memory) {
                for (pln = 0; pln < memory->nptrs; pln++) {
                    if (memory->cl_type == CL_MEM_OBJECT_BUFFER) {
                        gettimeofday(&start1, NULL);
                        err = clEnqueueReadBuffer(context->queues[plidx][didx],
                            memory->hdls[pln],
                            CL_TRUE, 0, ownComputeMemorySize(memory, pln),
                            memory->ptrs[pln],
                            0, NULL, NULL);
                        gettimeofday(&end, NULL);

                        double costTime = ((double)end.tv_sec * 1000.0 + (double)end.tv_usec / 1000.0)
                                            - ((double)start1.tv_sec * 1000.0 + (double)start1.tv_usec / 1000.0);

                        printf("opencl read DMA %f ms\n", costTime);
                        CL_ERROR_MSG(err, "clEnqueueReadBuffer");
                    } else if (memory->cl_type == CL_MEM_OBJECT_IMAGE2D) {
                        vx_rectangle_t rect = {0};
                        vx_image image = (vx_image)ref;
                        vxGetValidRegionImage(image, &rect);
                        size_t origin[3] = {rect.start_x, rect.start_y, 0};
                        size_t region[3] = {rect.end_x-rect.start_x, rect.end_y-rect.start_y, 1};
                        /* set the work dimensions */
                        work_dim[0] = rect.end_x-rect.start_x;
                        work_dim[1] = rect.end_y-rect.start_y;
                        err = clEnqueueReadImage(context->queues[plidx][didx],
                                                  memory->hdls[pln],
                                                  CL_TRUE,
                                                  origin, region,
                                                  memory->strides[pln][VX_DIM_Y],
                                                  0,
                                                  memory->ptrs[pln],
                                                  1, &node->base.event,
                                                  &ref->event);
                        CL_ERROR_MSG(err, "clEnqueueReadImage");
                        VX_PRINT(VX_ZONE_INFO, "Reading Image wd={%zu,%zu}\n", work_dim[0], work_dim[1]);
                    }
                }
            }
        }
    }
    re = 0;
    for (pidx = 0; pidx < num; pidx++) {
        vx_reference ref = node->parameters[pidx];
        vx_enum dir = node->kernel->signature.directions[pidx];
        if (dir == VX_OUTPUT || dir == VX_BIDIRECTIONAL) {
            memcpy(&readEvents[re++],&ref->event, sizeof(cl_event));
        }
    }
    err = clFlush(context->queues[plidx][didx]);
    gettimeofday(&end, NULL);

    double costTime1 = ((double)end.tv_sec * 1000.0 + (double)end.tv_usec / 1000.0)
                        - ((double)start.tv_sec * 1000.0 + (double)start.tv_usec / 1000.0);

    printf("box3x3 core %f ms\n", costTime1);
    CL_ERROR_MSG(err, "Flush");
    VX_PRINT(VX_ZONE_TARGET, "Waiting for read events!\n");
    clWaitForEvents(re, readEvents);
    if (err == CL_SUCCESS)
        status = VX_SUCCESS;
//exit:
    VX_PRINT(VX_ZONE_API, "%s exiting %d\n", __FUNCTION__, status);
    return status;
}

