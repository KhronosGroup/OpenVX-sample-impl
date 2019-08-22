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

static
vx_param_description_t phase_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK vxPhaseKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
} /* vxPhaseKernel() */

static vx_status VX_CALLBACK vxPhaseInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (index == 0 || index == 1)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_S16 || format == VX_DF_IMAGE_F32)
            {
                if (index == 0)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    vx_parameter param0 = vxGetParameterByIndex(node, index);
                    vx_image input0 = 0;

                    vxQueryParameter(param0, VX_PARAMETER_REF, &input0, sizeof(input0));
                    if (input0)
                    {
                        vx_uint32 width0 = 0, height0 = 0, width1 = 0, height1 = 0;
                        vxQueryImage(input0, VX_IMAGE_WIDTH, &width0, sizeof(width0));
                        vxQueryImage(input0, VX_IMAGE_HEIGHT, &height0, sizeof(height0));
                        vxQueryImage(input, VX_IMAGE_WIDTH, &width1, sizeof(width1));
                        vxQueryImage(input, VX_IMAGE_HEIGHT, &height1, sizeof(height1));

                        if (width0 == width1 && height0 == height1)
                            status = VX_SUCCESS;

                        vxReleaseImage(&input0);
                    }

                    vxReleaseParameter(&param0);
                }
            }

            vxReleaseImage(&input);
        }

        vxReleaseParameter(&param);
    }

    return status;
}

static vx_status VX_CALLBACK vxPhaseOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (index == 2)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, 0);

        vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
        if (input)
        {
            vx_uint32   width = 0;
            vx_uint32   height = 0;
            vx_df_image format = 0;

            vxQueryImage(input, VX_IMAGE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_HEIGHT, &height, sizeof(height));
            vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));

            ptr->type = VX_TYPE_IMAGE;
            ptr->dim.image.format = VX_DF_IMAGE_U8;
            ptr->dim.image.width  = width;
            ptr->dim.image.height = height;

            status = VX_SUCCESS;

            vxReleaseImage(&input);
        }

        vxReleaseParameter(&param);
    }

    return status;
}

vx_cl_kernel_description_t phase_kernel =
{
    {
    VX_KERNEL_PHASE,
    "org.khronos.openvx.phase",
    vxPhaseKernel,
    phase_kernel_params, dimof(phase_kernel_params),
    NULL,
    vxPhaseInputValidator,
    vxPhaseOutputValidator,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_phase.cl",
    "vx_phase",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};


