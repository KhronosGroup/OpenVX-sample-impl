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

    vx_reference ref;

    // Input src
    ref = node->parameters[0];
    vx_memory_t *memory = &((vx_image)ref)->memory;

    vx_size in_step_x = 1;
    vx_size in_step_y = 1;
    vx_size in_offset_first_element_in_bytes = 0;

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &memory->hdls[pln]);

    //stride_x, step_x, stride_y, step_y
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_X]);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &in_step_x);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_Y]);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &in_step_y);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &in_offset_first_element_in_bytes);
    VX_PRINT(VX_ZONE_INFO, "Setting vx_image as Buffer with 5 parameters\n");

    vx_int32 src_width = memory->dims[pln][VX_DIM_X];
    vx_int32 src_height = memory->dims[pln][VX_DIM_Y];

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

    //Set Output
    ref = node->parameters[3];
    memory = &((vx_image)ref)->memory;

    vx_size out_step_x = 4;
    vx_size out_step_y = memory->strides[pln][VX_DIM_Y];
    vx_size out_offset_first_element_in_bytes = 0;

    /* set the work dimensions */
    work_dim[0] = memory->dims[pln][VX_DIM_X] / 4;
    work_dim[1] = memory->dims[pln][VX_DIM_Y];

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &memory->hdls[pln]);

    //stride_x, step_x, stride_y, step_y
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_X]);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &out_step_x);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &memory->strides[pln][VX_DIM_Y]);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &out_step_y);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &out_offset_first_element_in_bytes);
    VX_PRINT(VX_ZONE_INFO, "Setting vx_image as Buffer with 5 parameters\n");

    int width = memory->dims[pln][VX_DIM_X];
    int height = memory->dims[pln][VX_DIM_Y];
    //width, height
    err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_int32), &src_width);
    err = clSetKernelArg(vxclk->kernels[plidx], argidx++, sizeof(vx_int32), &src_height);

    CL_ERROR_MSG(err, "clSetKernelArg");

    vx_matrix mask = (vx_matrix)parameters[1];

    vx_size matrix_size = 9;

    //vx_float32 *m = (vx_float32 *)malloc(matrix_size * sizeof(vx_float32));
    vx_float32 m[9];

    status |= vxCopyMatrix(mask, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    cl_mem mat = clCreateBuffer(context->global[0], CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, matrix_size * sizeof(vx_float32), m, &err);

    err = clEnqueueWriteBuffer(context->queues[plidx][didx],
        mat,
        CL_TRUE,
        0,
        matrix_size * sizeof(vx_float32),
        m,
        0,
        NULL,
        NULL);

    err = clSetKernelArg(kernel, argidx++, sizeof(cl_mem), &mat);

    //Set bordermode
    vx_border_t bordermode;
    status = vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));
    //Set const value for constant boder 
    uint8_t const_vaule = bordermode.constant_value.U8;
    err = clSetKernelArg(kernel, argidx++, sizeof(uint8_t), &const_vaule);

    //Set type
    vx_scalar stype = (vx_scalar)parameters[2];
    vx_int32 type = 0;
    status |= vxCopyScalar(stype, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &type);

    we = 0;

    // Input src
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

    memcpy(&readEvents[re++], &ref->event, sizeof(cl_event));

    err = clFlush(context->queues[plidx][didx]);
    CL_ERROR_MSG(err, "Flush");
    VX_PRINT(VX_ZONE_TARGET, "Waiting for read events!\n");
    clWaitForEvents(re, readEvents);
    if (err == CL_SUCCESS)
        status = VX_SUCCESS;

    VX_PRINT(VX_ZONE_API, "%s exiting %d\n", __FUNCTION__, status);

    clReleaseMemObject(mat);

    return status;
}

static vx_status vxWarpInputValidator(vx_node node, vx_uint32 index, vx_size mat_columns)
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
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_matrix matrix;
            vxQueryParameter(param, VX_PARAMETER_REF, &matrix, sizeof(matrix));
            if (matrix)
            {
                vx_enum data_type = 0;
                vx_size rows = 0ul, columns = 0ul;
                vxQueryMatrix(matrix, VX_MATRIX_TYPE, &data_type, sizeof(data_type));
                vxQueryMatrix(matrix, VX_MATRIX_ROWS, &rows, sizeof(rows));
                vxQueryMatrix(matrix, VX_MATRIX_COLUMNS, &columns, sizeof(columns));
                if ((data_type == VX_TYPE_FLOAT32) && (columns == mat_columns) && (rows == 3))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseMatrix(&matrix);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vx_enum interp = 0;
                    vxCopyScalar(scalar, &interp, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((interp == VX_INTERPOLATION_NEAREST_NEIGHBOR) ||
                        (interp == VX_INTERPOLATION_BILINEAR))
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
    }
    return status;
}

static vx_status VX_CALLBACK vxWarpAffineInputValidator(vx_node node, vx_uint32 index)
{
    return vxWarpInputValidator(node, index, 2);
}

static vx_status VX_CALLBACK vxWarpPerspectiveInputValidator(vx_node node, vx_uint32 index)
{
    return vxWarpInputValidator(node, index, 3);
}

static vx_status VX_CALLBACK vxWarpOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS)
        {
            vx_image dst = 0;
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            if (dst)
            {
                vx_uint32 w1 = 0, h1 = 0;
                vx_df_image f1 = VX_DF_IMAGE_VIRT;

                vxQueryImage(dst, VX_IMAGE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(dst, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(dst, VX_IMAGE_FORMAT, &f1, sizeof(f1));
                /* output can not be virtual */
                if ((w1 != 0) && (h1 != 0) && (f1 == VX_DF_IMAGE_U8))
                {
                    /* fill in the meta data with the attributes so that the checker will pass */
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = VX_DF_IMAGE_U8;
                    ptr->dim.image.width = w1;
                    ptr->dim.image.height = h1;
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&dst);
            }
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

static vx_param_description_t warp_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_MATRIX, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

static vx_status VX_CALLBACK vxWarpAffineKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    printf("OpenCL WarpAffine\n");

    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
}

static vx_status VX_CALLBACK vxWarpPerspectiveKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    printf("OpenCL WarpPerspective\n");

    vx_status status = vxclCallOpenCLKernel(node, parameters, num);

    return status;
}

vx_cl_kernel_description_t warp_affine_kernel = {
    {
        VX_KERNEL_WARP_AFFINE,
        "org.khronos.openvx.warp_affine",
        vxWarpAffineKernel,
        warp_kernel_params, dimof(warp_kernel_params),
        NULL,
        vxWarpAffineInputValidator,
        vxWarpOutputValidator,
        NULL,
        NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_warp_affine.cl",
    "warp_affine",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};

vx_cl_kernel_description_t warp_perspective_kernel = {
    {
        VX_KERNEL_WARP_PERSPECTIVE,
        "org.khronos.openvx.warp_perspective",
        vxWarpPerspectiveKernel,
        warp_kernel_params, dimof(warp_kernel_params),
        NULL,
        vxWarpPerspectiveInputValidator,
        vxWarpOutputValidator,
        NULL,
        NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_warp_perspective.cl",
    "warp_perspective",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};
