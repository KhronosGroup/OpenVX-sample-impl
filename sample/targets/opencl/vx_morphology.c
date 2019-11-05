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
#include <VX/vx_helper.h>
#include <vx_support.h>

#include "vx_interface.h"

static vx_status VX_CALLBACK vxclCallOpenCLKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    vx_context context = node->base.context;

    vx_cl_kernel_description_t *vxclk = vxclFindKernel(node->kernel->enumeration);
    vx_uint32 pidx, pln, didx, plidx, argidx;
    cl_int err = 0;
    size_t off_dim[3] = { 0,0,0 };
    size_t work_dim[3];

    cl_event writeEvents[VX_INT_MAX_PARAMS];
    cl_event readEvents[VX_INT_MAX_PARAMS];
    cl_int we = 0, re = 0;

    // determine which platform to use
    plidx = 0;

    // determine which device to use
    didx = 0;

    cl_kernel kernel = vxclk->kernels[plidx];

    pln = 0;

    argidx = 0;

    //Set Input
    vx_reference ref = node->parameters[0];
    vx_enum dir = node->kernel->signature.directions[0];
    vx_memory_t *memory = &((vx_image)ref)->memory;

    /* set the work dimensions */
    work_dim[0] = memory->dims[pln][VX_DIM_X];
    work_dim[1] = memory->dims[pln][VX_DIM_Y];

    //stride_x, stride_y
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_X]);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_Y]);
    VX_PRINT(VX_ZONE_INFO, "Setting vx_image as Buffer with 2 parameters\n");

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &memory->hdls[pln]);
    CL_ERROR_MSG(err, "clSetKernelArg");

    err = clEnqueueWriteBuffer(context->queues[plidx][didx],
        memory->hdls[pln],
        CL_TRUE,
        0,
        ownComputeMemorySize(memory, pln),
        memory->ptrs[pln],
        0,
        NULL,
        &ref->event);

    //Set bordermode
    vx_border_t bordermode;
    status = vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));

    int border_mode = bordermode.mode;
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &border_mode);

    //Set const value for constant boder 
    uint8_t const_vaule = bordermode.constant_value.U8;
    err = clSetKernelArg(kernel, argidx++, sizeof(uint8_t), &const_vaule);

    //Set Output
    ref = node->parameters[1];
    memory = &((vx_image)ref)->memory;

    /* set the work dimensions */
    work_dim[0] = memory->dims[pln][VX_DIM_X];
    work_dim[1] = memory->dims[pln][VX_DIM_Y];

    //stride_x, stride_y
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_X]);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_Y]);
    VX_PRINT(VX_ZONE_INFO, "Setting vx_image as Buffer with 2 parameters\n");

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &memory->hdls[pln]);
    CL_ERROR_MSG(err, "clSetKernelArg");

    we = 0;
    for (pidx = 0; pidx < num; pidx++)
    {
        vx_reference ref = node->parameters[pidx];
        vx_enum dir = node->kernel->signature.directions[pidx];
        if (dir == VX_INPUT || dir == VX_BIDIRECTIONAL)
        {
            memcpy(&writeEvents[we++], &ref->event, sizeof(cl_event));
        }
    }


    err = clEnqueueNDRangeKernel(context->queues[plidx][didx],
        kernel,
        2,
        off_dim,
        work_dim,
        NULL,
        we, writeEvents, &node->base.event);

    clFinish(context->queues[plidx][didx]);

    CL_ERROR_MSG(err, "clEnqueueNDRangeKernel");

    pln = 0;

    /* enqueue a read on all output data */
    ref = node->parameters[1];

    memory = &((vx_image)ref)->memory;

    err = clEnqueueReadBuffer(context->queues[plidx][didx],
        memory->hdls[pln],
        CL_TRUE, 0, ownComputeMemorySize(memory, pln),
        memory->ptrs[pln],
        0, NULL, NULL);

    CL_ERROR_MSG(err, "clEnqueueReadBuffer");

    clFinish(context->queues[plidx][didx]);

    re = 0;
    for (pidx = 0; pidx < num; pidx++)
    {
        vx_reference ref = node->parameters[pidx];
        vx_enum dir = node->kernel->signature.directions[pidx];
        if (dir == VX_OUTPUT || dir == VX_BIDIRECTIONAL)
        {
            memcpy(&readEvents[re++], &ref->event, sizeof(cl_event));
        }
    }
    err = clFlush(context->queues[plidx][didx]);
    CL_ERROR_MSG(err, "Flush");
    VX_PRINT(VX_ZONE_TARGET, "Waiting for read events!\n");
    clWaitForEvents(re, readEvents);
    if (err == CL_SUCCESS)
        status = VX_SUCCESS;

    VX_PRINT(VX_ZONE_API, "%s exiting %d\n", __FUNCTION__, status);
    return status;
}

static vx_status VX_CALLBACK vxErode3x3Kernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
}


static vx_status VX_CALLBACK vxDilate3x3Kernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
}

static vx_status VX_CALLBACK vxMorphologyInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxMorphologyOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0); /* we reference the input image */
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if (input)
            {
                vx_uint32 width = 0, height = 0;
                vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U8;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_param_description_t morphology_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_cl_kernel_description_t erode3x3_kernel = {
    {
    VX_KERNEL_ERODE_3x3,
    "org.khronos.openvx.erode_3x3",
    vxErode3x3Kernel,
    morphology_kernel_params, dimof(morphology_kernel_params),
    NULL,
    vxMorphologyInputValidator,
    vxMorphologyOutputValidator,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_erode3x3.cl",
    "vx_erode3x3",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};

vx_cl_kernel_description_t dilate3x3_kernel = {
    {
    VX_KERNEL_DILATE_3x3,
    "org.khronos.openvx.dilate_3x3",
    vxDilate3x3Kernel,
    morphology_kernel_params, dimof(morphology_kernel_params),
    NULL,
    vxMorphologyInputValidator,
    vxMorphologyOutputValidator,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_dilate3x3.cl",
    "vx_dilate3x3",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};


