/*

 * Copyright (c) 2016-2017 The Khronos Group Inc.
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

#define C_MAX_NONLINEAR_DIM (9)

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

    // Input function
    vx_reference ref = node->parameters[0];
    vx_value_t value; // largest platform atomic
    vx_size size = 0ul;
    vx_scalar sc = (vx_scalar)ref;
    vx_enum stype = VX_TYPE_INVALID;
    vxCopyScalar(sc, &value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    vxQueryScalar(sc, VX_SCALAR_TYPE, &stype, sizeof(stype));
    size = ownSizeOfType(stype);
    err = clSetKernelArg(kernel, argidx++, size, &value);


    // Input src
    ref = node->parameters[1];
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


    // Input mask
    ref = node->parameters[2];
    memory = &((vx_matrix)ref)->memory;

    size = ownComputeMemorySize(memory, pln);

    memory->hdls[pln] = clCreateBuffer(context->global[0], CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, size, memory->ptrs[pln], &err);
    err = clEnqueueWriteBuffer(context->queues[plidx][didx],
        memory->hdls[pln],
        CL_TRUE,
        0,
        size,
        memory->ptrs[pln],
        0,
        NULL,
        NULL);

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &memory->hdls[pln]);
    CL_ERROR_MSG(err, "clSetKernelArg");

    // Origin matrix
    vx_matrix mask = (vx_matrix)parameters[2];
    vx_coordinates2d_t origin;
    status |= vxQueryMatrix(mask, VX_MATRIX_ORIGIN, &origin, sizeof(origin));

    vx_matrix mat = (vx_matrix)ref;
    vx_size rx0 = origin.x;
    vx_size ry0 = origin.y;
    vx_size rx1 = mat->columns - origin.x - 1;
    vx_size ry1 = mat->rows - origin.y - 1;

    err = clSetKernelArg(kernel, argidx++, sizeof(vx_size), &rx0);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_size), &ry0);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_size), &rx1);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_size), &ry1);

    vx_uint8 m[C_MAX_NONLINEAR_DIM * C_MAX_NONLINEAR_DIM];
    status |= vxCopyMatrix(mask, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    int mask_index = 0;
    int count_mask = 0;
    for (int r = 0; r < mat->rows; ++r)
    {
        for (int c = 0; c < mat->columns; ++c, ++mask_index)
        {
            if (m[mask_index])
                ++count_mask;
        }
    }

    err = clSetKernelArg(kernel, argidx++, sizeof(int), &mat->rows);
    err = clSetKernelArg(kernel, argidx++, sizeof(int), &count_mask);

    //Set bordermode
    vx_border_t bordermode;
    status = vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));

    int border_mode = bordermode.mode;
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &border_mode);

    //Set const value for constant boder 
    uint8_t const_vaule = bordermode.constant_value.U8;
    err = clSetKernelArg(kernel, argidx++, sizeof(uint8_t), &const_vaule);


    //Set Output
    ref = node->parameters[3];
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

    // Input src
    ref = node->parameters[1];
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

    pln = 0;

    /* enqueue a read on all output data */
    ref = node->parameters[3];

    memory = &((vx_image)ref)->memory;

    err = clEnqueueReadBuffer(context->queues[plidx][didx],
        memory->hdls[pln],
        CL_TRUE, 0, ownComputeMemorySize(memory, pln),
        memory->ptrs[pln],
        0, NULL, NULL);

    CL_ERROR_MSG(err, "clEnqueueReadBuffer");

    clFinish(context->queues[plidx][didx]);

    re = 0;

    ref = node->parameters[3];

    memcpy(&readEvents[re++], &ref->event, sizeof(cl_event));

    err = clFlush(context->queues[plidx][didx]);
    CL_ERROR_MSG(err, "Flush");
    VX_PRINT(VX_ZONE_TARGET, "Waiting for read events!\n");
    clWaitForEvents(re, readEvents);
    if (err == CL_SUCCESS)
        status = VX_SUCCESS;

    VX_PRINT(VX_ZONE_API, "%s exiting %d\n", __FUNCTION__, status);
    return status;
}

static vx_status VX_CALLBACK vxNonLinearFilterKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
}

static vx_status VX_CALLBACK vxNonLinearFilterInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_scalar scalar = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
        if (scalar)
        {
            vx_enum stype = 0;
            vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
            if (stype == VX_TYPE_ENUM)
            {
                vx_enum function = 0;
                vxCopyScalar(scalar, &function, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                if ((function == VX_NONLINEAR_FILTER_MEDIAN) ||
                    (function == VX_NONLINEAR_FILTER_MIN) ||
                    (function == VX_NONLINEAR_FILTER_MAX))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_VALUE;
                }
            }
            else
            {
                status = VX_ERROR_INVALID_TYPE;
            }
            vxReleaseScalar(&scalar);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
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
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_matrix matrix;
            vxQueryParameter(param, VX_PARAMETER_REF, &matrix, sizeof(matrix));
            if (matrix)
            {
                vx_enum data_type = 0;
                vx_size cols = 0, rows = 0;
                vxQueryMatrix(matrix, VX_MATRIX_TYPE, &data_type, sizeof(data_type));
                vxQueryMatrix(matrix, VX_MATRIX_COLUMNS, &cols, sizeof(cols));
                vxQueryMatrix(matrix, VX_MATRIX_ROWS, &rows, sizeof(rows));
                if ((rows <= VX_INT_MAX_NONLINEAR_DIM) &&
                    (cols <= VX_INT_MAX_NONLINEAR_DIM) &&
                    (data_type == VX_TYPE_UINT8))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseMatrix(&matrix);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxNonLinearFilterOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, 1); /* we reference the input image */
        if (param)
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

static vx_param_description_t filter_kernel_params[] = {
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_MATRIX, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
};

vx_cl_kernel_description_t nonlinearfilter_kernel = {
    {
    VX_KERNEL_NON_LINEAR_FILTER,
    "org.khronos.openvx.non_linear_filter",
    vxNonLinearFilterKernel,
    filter_kernel_params, dimof(filter_kernel_params),
    NULL,
    vxNonLinearFilterInputValidator,
    vxNonLinearFilterOutputValidator,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_nonlinearfilter.cl",
    "vx_nonlinearfilter",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};
