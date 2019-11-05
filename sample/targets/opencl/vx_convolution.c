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

#define C_MAX_CONVOLUTION_DIM (15)

#if (C_MAX_CONVOLUTION_DIM != VX_INT_MAX_CONVOLUTION_DIM)
#if defined(_WIN32)
#pragma error("C Model does not support VX required Convolution Size")
#elif defined(__GNUC__)
#error "C Model does not support VX required Convolution Size"
#endif
#endif


static vx_status VX_CALLBACK vxclCallOpenCLKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    vx_context context = node->base.context;

    vx_cl_kernel_description_t *vxclk = vxclFindKernel(node->kernel->enumeration);
    vx_uint32 pln, didx, plidx, argidx;
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

    //Set conv_mat
    vx_size conv_width, conv_height;
    vx_int16 _conv_mat[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = { 0 };
    vx_uint32 scale = 1;

    vx_convolution conv = (vx_convolution)parameters[1];

    status |= vxQueryConvolution(conv, VX_CONVOLUTION_COLUMNS, &conv_width, sizeof(conv_width));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_ROWS, &conv_height, sizeof(conv_height));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_SCALE, &scale, sizeof(scale));

    status |= vxCopyConvolutionCoefficients(conv, _conv_mat, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    err = clSetKernelArg(kernel, argidx++, sizeof(vx_uint32), &conv_width);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_uint32), &conv_height);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_uint32), &scale);

    short matrix_size = C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM * sizeof(short);

    cl_mem conv_mat = clCreateBuffer(context->global[0], CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, matrix_size, _conv_mat, &err);

    err = clEnqueueWriteBuffer(context->queues[plidx][didx],
        conv_mat,
        CL_TRUE,
        0,
        matrix_size,
        _conv_mat,
        0,
        NULL,
        NULL);

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &conv_mat);

    //Set Output
    ref = node->parameters[2];
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
    ref = node->parameters[0];
    memcpy(&writeEvents[we++], &ref->event, sizeof(cl_event));

    err = clEnqueueNDRangeKernel(context->queues[plidx][didx],
        kernel,
        2,
        off_dim,
        work_dim,
        NULL,
        we, writeEvents, &node->base.event);

    clFinish(context->queues[plidx][didx]);

    CL_ERROR_MSG(err, "clEnqueueNDRangeKernel");

    /* enqueue a read on all output data */
    ref = node->parameters[2];

    memory = &((vx_image)ref)->memory;

    err = clEnqueueReadBuffer(context->queues[plidx][didx],
        memory->hdls[pln],
        CL_TRUE, 0, ownComputeMemorySize(memory, pln),
        memory->ptrs[pln],
        0, NULL, NULL);

    CL_ERROR_MSG(err, "clEnqueueReadBuffer");

    clFinish(context->queues[plidx][didx]);

    re = 0;

    ref = node->parameters[2];
    memcpy(&readEvents[re++], &ref->event, sizeof(cl_event));

    err = clFlush(context->queues[plidx][didx]);
    CL_ERROR_MSG(err, "Flush");
    VX_PRINT(VX_ZONE_TARGET, "Waiting for read events!\n");
    clWaitForEvents(re, readEvents);
    if (err == CL_SUCCESS)
        status = VX_SUCCESS;

    VX_PRINT(VX_ZONE_API, "%s exiting %d\n", __FUNCTION__, status);

    clReleaseMemObject(conv_mat);

    return status;
}

static vx_status VX_CALLBACK vxConvolveInputValidator(vx_node node, vx_uint32 index)
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

#if defined(EXPERIMENTAL_USE_S16)
            if( (format == VX_DF_IMAGE_U8) || (format == VX_DF_IMAGE_S16) )
#else
            if (format == VX_DF_IMAGE_U8)
#endif
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    if (index == 1)
    {
        vx_image input = 0;
        vx_convolution conv = 0;

        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        vx_parameter param1 = vxGetParameterByIndex(node, index);

        vxQueryParameter(param0, VX_PARAMETER_REF, &input, sizeof(input));
        vxQueryParameter(param1, VX_PARAMETER_REF, &conv, sizeof(conv));
        if (input && conv)
        {
            vx_uint32 width = 0;
            vx_uint32 height = 0;
            vx_size dims[2] = { 0, 0 };

            vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));

            vxQueryConvolution(conv, VX_CONVOLUTION_COLUMNS, &dims[0], sizeof(dims[0]));
            vxQueryConvolution(conv, VX_CONVOLUTION_ROWS, &dims[1], sizeof(dims[1]));

            if ((dims[0] <= VX_INT_MAX_CONVOLUTION_DIM) &&
                (dims[1] <= VX_INT_MAX_CONVOLUTION_DIM) &&
                (width >= dims[0]) &&
                (height >= dims[1]))
            {
                status = VX_SUCCESS;
            }

            vxReleaseImage(&input);
            vxReleaseConvolution(&conv);
        }

        vxReleaseParameter(&param0);
        vxReleaseParameter(&param1);
    }

    return status;
}

static vx_status VX_CALLBACK vxConvolveOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter params[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, index),
        };
        if ((vxGetStatus((vx_reference)params[0]) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)params[1]) == VX_SUCCESS))
        {
            vx_image input = 0;
            vx_image output = 0;
            vxQueryParameter(params[0], VX_PARAMETER_REF, &input, sizeof(input));
            vxQueryParameter(params[1], VX_PARAMETER_REF, &output, sizeof(output));
            if (input && output)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = 0;
                vx_df_image output_format = 0;
                vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));

                vxQueryImage(output, VX_IMAGE_FORMAT, &output_format, sizeof(output_format));

                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = output_format == VX_DF_IMAGE_U8 ? VX_DF_IMAGE_U8 : VX_DF_IMAGE_S16;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;

                vxReleaseImage(&input);
                vxReleaseImage(&output);
            }
            vxReleaseParameter(&params[0]);
            vxReleaseParameter(&params[1]);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxConvolveKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    printf("OpenCL Convolve\n");

    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
}

static vx_param_description_t convolution_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_CONVOLUTION, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_cl_kernel_description_t convolution_kernel = {
    {
    VX_KERNEL_CUSTOM_CONVOLUTION,
    "org.khronos.openvx.custom_convolution",
    vxConvolveKernel,
    convolution_kernel_params, dimof(convolution_kernel_params),
    NULL,
    vxConvolveInputValidator,
    vxConvolveOutputValidator,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_convolve.cl",
    "vx_Convolve",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};

