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
 * \brief The Warp Affine and Perspective Kernels
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
            if ((format == VX_DF_IMAGE_U8) ||
                (format == VX_DF_IMAGE_U1 && mat_columns == 2))
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
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter mtx_param = vxGetParameterByIndex(node, 1);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if ((vxGetStatus((vx_reference)src_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)mtx_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS))
        {
            vx_matrix mtx = 0;
            vx_image src = 0, dst = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            vxQueryParameter(mtx_param, VX_PARAMETER_REF, &mtx, sizeof(mtx));
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            if (src && mtx && dst)
            {
                vx_size mtx_cols = 0ul;
                vx_uint32 w1 = 0, h1 = 0;
                vx_df_image f0 = VX_DF_IMAGE_VIRT, f1 = VX_DF_IMAGE_VIRT;

                vxQueryImage(dst, VX_IMAGE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(dst, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(src, VX_IMAGE_FORMAT, &f0, sizeof(f0));
                vxQueryImage(dst, VX_IMAGE_FORMAT, &f1, sizeof(f1));
                vxQueryMatrix(mtx, VX_MATRIX_COLUMNS, &mtx_cols, sizeof(mtx_cols));
                /* output can not be virtual */
                // TODO handle affive vx perspective
                if ( (w1 != 0) && (h1 != 0) && (f0 == f1) &&
                     (f1 == VX_DF_IMAGE_U8 || (f1 == VX_DF_IMAGE_U1 && mtx_cols == 2)) )
                {
                    /* fill in the meta data with the attributes so that the checker will pass */
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = f1;
                    ptr->dim.image.width = w1;
                    ptr->dim.image.height = h1;
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&src);
                vxReleaseImage(&dst);
                vxReleaseMatrix(&mtx);
            }
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&mtx_param);
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

static vx_bool read_pixel_1u_C1(void *base, vx_imagepatch_addressing_t *addr, vx_float32 x, vx_float32 y,
                                const vx_border_t *borders, vx_uint8 *pxl_group, vx_uint32 x_dst, vx_uint32 shift_x_u1)
{
    vx_bool out_of_bounds = (x < shift_x_u1 || y < 0 || x >= addr->dim_x || y >= addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 bpixel;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pxl_group = (*pxl_group & ~(1 << (x_dst % 8))) | ((borders->constant_value.U1 ? 1 : 0) << (x_dst % 8));
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < shift_x_u1 ? shift_x_u1 : x >= addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0          ? 0          : y >= addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (*(vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr) & (1 << (bx % 8))) >> (bx % 8);
    *pxl_group = (*pxl_group & ~(1 << (x_dst % 8))) | (bpixel << (x_dst % 8));

    return vx_true_e;
}

static vx_bool read_pixel_8u_C1(void *base, vx_imagepatch_addressing_t *addr,
                          vx_float32 x, vx_float32 y, const vx_border_t *borders, vx_uint8 *pixel)
{
    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= addr->dim_x || y >= addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 *bpixel;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_CONSTANT)
        {
            *pixel = borders->constant_value.U8;
            return vx_true_e;
        }
    }

    // bounded x/y
    bx = x < 0 ? 0 : x >= addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;

    bpixel = (vx_uint8*)vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = *bpixel;

    return vx_true_e;
}

typedef void (*transform_f)(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y);

static void transform_affine(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y)
{
    *src_x = dst_x * m[0] + dst_y * m[2] + m[4];
    *src_y = dst_x * m[1] + dst_y * m[3] + m[5];
}

static void transform_perspective(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y)
{
    vx_float32 z = dst_x * m[2] + dst_y * m[5] + m[8];

    *src_x = (dst_x * m[0] + dst_y * m[3] + m[6]) / z;
    *src_y = (dst_x * m[1] + dst_y * m[4] + m[7]) / z;
}

static vx_status vxWarpGeneric(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image,
                                  const vx_border_t *borders, transform_f transform)
{
    vx_status status = VX_SUCCESS;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t dst_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_uint32 dst_width;
    vx_uint32 dst_height;
    vx_rectangle_t src_rect;
    vx_rectangle_t dst_rect;
    vx_df_image format;

    vx_float32 m[9];
    vx_enum type = 0;

    vx_uint32 x = 0u;
    vx_uint32 y = 0u;

    vx_map_id src_map_id = 0;
    vx_map_id dst_map_id = 0;

    status |= vxQueryImage(dst_image, VX_IMAGE_WIDTH, &dst_width, sizeof(dst_width));
    status |= vxQueryImage(dst_image, VX_IMAGE_HEIGHT, &dst_height, sizeof(dst_height));
    status |= vxQueryImage(dst_image, VX_IMAGE_FORMAT, &format, sizeof(format));

    status |= vxGetValidRegionImage(src_image, &src_rect);
    vx_uint32 shift_x_u1 = src_rect.start_x % 8;  // Bit-shift offset for U1 images

    dst_rect.start_x = 0;
    dst_rect.start_y = 0;
    dst_rect.end_x   = dst_width;
    dst_rect.end_y   = dst_height;

    status |= vxMapImagePatch(src_image, &src_rect, 0, &src_map_id, &src_addr, &src_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    status |= vxMapImagePatch(dst_image, &dst_rect, 0, &dst_map_id, &dst_addr, &dst_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, 0);

    status |= vxCopyMatrix(matrix, m, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxCopyScalar(stype, &type, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if (status == VX_SUCCESS)
    {
        for (y = 0u; y < dst_addr.dim_y; y++)
        {
            for (x = 0u; x < dst_addr.dim_x; x++)
            {
                vx_uint8 *dst = (vx_uint8*)vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);

                vx_float32 xf;
                vx_float32 yf;
                transform(x, y, m, &xf, &yf);
                xf -= (vx_float32)src_rect.start_x;
                yf -= (vx_float32)src_rect.start_y;
                if (format == VX_DF_IMAGE_U1)   // Add bit-shift offset
                    xf += shift_x_u1;

                if (type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
                {
                    if (format == VX_DF_IMAGE_U1)
                        read_pixel_1u_C1(src_base, &src_addr, xf, yf, borders, dst, x, shift_x_u1);
                    else
                        read_pixel_8u_C1(src_base, &src_addr, xf, yf, borders, dst);
                }
                else if (type == VX_INTERPOLATION_BILINEAR)
                {
                    vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;
                    vx_bool defined = vx_true_e;
                    if (format == VX_DF_IMAGE_U1)
                    {
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf), floorf(yf), borders, &tl, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf), borders, &tr, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf), floorf(yf) + 1, borders, &bl, 0, shift_x_u1);
                        defined &= read_pixel_1u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf) + 1, borders, &br, 0, shift_x_u1);
                    }
                    else
                    {
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf), floorf(yf), borders, &tl);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf), borders, &tr);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf), floorf(yf) + 1, borders, &bl);
                        defined &= read_pixel_8u_C1(src_base, &src_addr, floorf(xf) + 1, floorf(yf) + 1, borders, &br);
                    }

                    if (defined)
                    {
                        vx_float32 ar = xf - floorf(xf);
                        vx_float32 ab = yf - floorf(yf);
                        vx_float32 al = 1.0f - ar;
                        vx_float32 at = 1.0f - ab;

                        if (format == VX_DF_IMAGE_U1)
                        {
                            // Arithmetic rounding instead of truncation for U1 images
                            vx_uint8 dst_val = (vx_uint8)(tl * al * at + tr * ar * at + bl * al * ab + br * ar * ab + 0.5);
                            *dst = (*dst & ~(1 << (x % 8))) | (dst_val << (x % 8));
                        }
                        else
                        {
                            *dst = (vx_uint8)(tl * al * at + tr * ar * at + bl * al * ab + br * ar * ab);
                        }
                    }
                }
            }
        }

        /*! \todo compute maximum area rectangle */
    }

    status |= vxCopyMatrix(matrix, m, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    status |= vxUnmapImagePatch(src_image, src_map_id);
    status |= vxUnmapImagePatch(dst_image, dst_map_id);

    return status;
}

static vx_status VX_CALLBACK vxWarpAffineKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_image  src_image = (vx_image) parameters[0];
    vx_matrix matrix    = (vx_matrix)parameters[1];
    vx_scalar stype     = (vx_scalar)parameters[2];
    vx_image  dst_image = (vx_image) parameters[3];

    vx_border_t borders;
    vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
    vx_df_image format;
    vx_rectangle_t src_rect;
    vx_status status = vxQueryImage(dst_image, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(src_image, &src_rect);
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1 || src_rect.start_x != 0)
        return vxWarpGeneric(src_image, matrix, stype, dst_image, &borders, transform_affine);
    else
        return vxclCallOpenCLKernel(node, parameters, num);
}

static vx_status VX_CALLBACK vxWarpPerspectiveKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_image  src_image = (vx_image) parameters[0];
    vx_matrix matrix    = (vx_matrix)parameters[1];
    vx_scalar stype     = (vx_scalar)parameters[2];
    vx_image  dst_image = (vx_image) parameters[3];

    vx_border_t borders;
    vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
    vx_df_image format;
    vx_rectangle_t src_rect;
    vx_status status = vxQueryImage(dst_image, VX_IMAGE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(src_image, &src_rect);
    if (status != VX_SUCCESS)
        return status;
    else if (format == VX_DF_IMAGE_U1 || src_rect.start_x != 0)
        return vxWarpGeneric(src_image, matrix, stype, dst_image, &borders, transform_perspective);
    else
        return vxclCallOpenCLKernel(node, parameters, num);
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
