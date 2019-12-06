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
 * \brief The Gradient Kernels (Base)
 * \author Erik Rainey <erik.rainey@gmail.com>
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

    //Set grad_x
    ref = node->parameters[1];
    memory = &((vx_image)ref)->memory;

    /* set the work dimensions */
    work_dim[0] = memory->dims[pln][VX_DIM_X];
    work_dim[1] = memory->dims[pln][VX_DIM_Y];

    int stride_x = memory->strides[pln][VX_DIM_X] / 2;
    int stride_y = memory->strides[pln][VX_DIM_Y] / 2;

    //stride_x, stride_y
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &stride_x);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &stride_y);
    VX_PRINT(VX_ZONE_INFO, "Setting vx_image as Buffer with 2 parameters\n");

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &memory->hdls[pln]);
    CL_ERROR_MSG(err, "clSetKernelArg");


    //Set grad_y
    ref = node->parameters[2];
    memory = &((vx_image)ref)->memory;

    /* set the work dimensions */
    work_dim[0] = memory->dims[pln][VX_DIM_X];
    work_dim[1] = memory->dims[pln][VX_DIM_Y];

    int stride_x1 = memory->strides[pln][VX_DIM_X] / 2;
    int stride_y1 = memory->strides[pln][VX_DIM_Y] / 2;

    //stride_x, stride_y
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &stride_x1);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &stride_y1);
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

static vx_param_description_t sobel3x3_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static vx_status VX_CALLBACK ownSobel3x3Kernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
} /* ownSobel3x3Kernel() */

static
vx_status VX_CALLBACK own_sobel3x3_validator(vx_node node, const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(sobel3x3_kernel_params) && NULL != metas)
    {
        vx_parameter param1 = vxGetParameterByIndex(node, 0);
        vx_parameter param2 = vxGetParameterByIndex(node, 1);
        vx_parameter param3 = vxGetParameterByIndex(node, 2);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            ( (VX_SUCCESS == vxGetStatus((vx_reference)param2)) || (VX_SUCCESS == vxGetStatus((vx_reference)param3)) ))
        {
            vx_uint32   src_width  = 0;
            vx_uint32   src_height = 0;
            vx_df_image src_format = 0;
            vx_image    input = 0;

            status = vxQueryParameter(param1, VX_PARAMETER_REF, &input, sizeof(input));

            status |= vxQueryImage(input, VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
            status |= vxQueryImage(input, VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
            status |= vxQueryImage(input, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));

            /* validate input image */
            if (VX_SUCCESS == status)
            {
                if (src_width >= 3 && src_height >= 3 && src_format == VX_DF_IMAGE_U8)
                    status = VX_SUCCESS;
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            /* validate output images */
            if (VX_SUCCESS == status)
            {
                vx_enum dst_format = VX_DF_IMAGE_S16;

                if (NULL == metas[1] && NULL == metas[2])
                    status = VX_ERROR_INVALID_PARAMETERS;

                if (VX_SUCCESS == status && NULL != metas[1])
                {
                    /* if optional parameter non NULL */
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                    status |= vxSetMetaFormatAttribute(metas[1], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                }

                if (VX_SUCCESS == status && NULL != metas[2])
                {
                    /* if optional parameter non NULL */
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                    status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));
                }
            }

            if (NULL != input)
                vxReleaseImage(&input);

            if (NULL != param1)
                vxReleaseParameter(&param1);

            if (NULL != param2)
                vxReleaseParameter(&param2);

            if (NULL != param3)
                vxReleaseParameter(&param3);
        }
    } /* if ptrs non NULL */

    return status;
} /* own_sobel3x3_validator() */


vx_cl_kernel_description_t sobel3x3_clkernel = {
    {
    VX_KERNEL_SOBEL_3x3,
    "org.khronos.openvx.sobel_3x3",
    ownSobel3x3Kernel,
    sobel3x3_kernel_params, dimof(sobel3x3_kernel_params),
    own_sobel3x3_validator,
    NULL,
    NULL,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_sobel3x3.cl",
    "vx_sobel3x3",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};

