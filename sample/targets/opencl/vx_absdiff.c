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
 * \brief The Absolute Difference Kernel.
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

    for (argidx = 0, pidx = 0; pidx < num; pidx++)
    {
        vx_reference ref = node->parameters[pidx];
        vx_enum dir = node->kernel->signature.directions[pidx];
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
        if (dir == VX_INPUT || dir == VX_BIDIRECTIONAL)
        {
            err = clEnqueueWriteBuffer(context->queues[plidx][didx],
                memory->hdls[pln],
                CL_TRUE,
                0,
                ownComputeMemorySize(memory, pln),
                memory->ptrs[pln],
                0,
                NULL,
                &ref->event);
        }
    }

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

    vx_reference ref;
    /* enqueue a read on all output data */
    ref = node->parameters[2];

    vx_memory_t *memory = NULL;

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

static vx_status VX_CALLBACK vxAbsDiffKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
}

static vx_status VX_CALLBACK vxAbsDiffInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0 )
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8
                || format == VX_DF_IMAGE_S16
#if defined(OPENVX_USE_S16)
                || format == VX_DF_IMAGE_U16
#endif
                )
                status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_image images[2];
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        vxQueryParameter(param[0], VX_PARAMETER_REF, &images[0], sizeof(images[0]));
        vxQueryParameter(param[1], VX_PARAMETER_REF, &images[1], sizeof(images[1]));
        if (images[0] && images[1])
        {
            vx_uint32 width[2], height[2];
            vx_df_image format[2];

            vxQueryImage(images[0], VX_IMAGE_WIDTH, &width[0], sizeof(width[0]));
            vxQueryImage(images[1], VX_IMAGE_WIDTH, &width[1], sizeof(width[1]));
            vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height[0], sizeof(height[0]));
            vxQueryImage(images[1], VX_IMAGE_HEIGHT, &height[1], sizeof(height[1]));
            vxQueryImage(images[0], VX_IMAGE_FORMAT, &format[0], sizeof(format[0]));
            vxQueryImage(images[1], VX_IMAGE_FORMAT, &format[1], sizeof(format[1]));
            if (width[0] == width[1] && height[0] == height[1] && format[0] == format[1])
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&images[0]);
            vxReleaseImage(&images[1]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    return status;
}

static vx_status VX_CALLBACK vxAbsDiffOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        if ((vxGetStatus((vx_reference)param[0]) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)param[1]) == VX_SUCCESS))
        {
            vx_image images[2];
            vxQueryParameter(param[0], VX_PARAMETER_REF, &images[0], sizeof(images[0]));
            vxQueryParameter(param[1], VX_PARAMETER_REF, &images[1], sizeof(images[1]));
            if (images[0] && images[1])
            {
                vx_uint32 width[2], height[2];
                vx_df_image format = 0;
                vxQueryImage(images[0], VX_IMAGE_FORMAT, &format, sizeof(format));
                vxQueryImage(images[0], VX_IMAGE_WIDTH, &width[0], sizeof(width[0]));
                vxQueryImage(images[1], VX_IMAGE_WIDTH, &width[1], sizeof(width[1]));
                vxQueryImage(images[0], VX_IMAGE_HEIGHT, &height[0], sizeof(height[0]));
                vxQueryImage(images[1], VX_IMAGE_HEIGHT, &height[1], sizeof(height[1]));
                if (width[0] == width[1] && height[0] == height[1] &&
                    (format == VX_DF_IMAGE_U8
                     || format == VX_DF_IMAGE_S16
#if defined(OPENVX_USE_S16)
                     || format == VX_DF_IMAGE_U16
#endif
                     ))
                {
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = format;
                    ptr->dim.image.width = width[0];
                    ptr->dim.image.height = height[1];
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&images[0]);
                vxReleaseImage(&images[1]);
            }
            vxReleaseParameter(&param[0]);
            vxReleaseParameter(&param[1]);
        }
    }
    return status;
}

static vx_param_description_t absdiff_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_cl_kernel_description_t absdiff_kernel = {
    {
    VX_KERNEL_ABSDIFF,
    "org.khronos.openvx.absdiff",
    vxAbsDiffKernel,
    absdiff_kernel_params, dimof(absdiff_kernel_params),
    NULL,
    vxAbsDiffInputValidator,
    vxAbsDiffOutputValidator,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_absdiff.cl",
    "vx_absdiff",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};

