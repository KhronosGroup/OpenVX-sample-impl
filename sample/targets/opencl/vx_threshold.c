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
 * \brief The Thresholding Kernel.
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

    //Set threshold
    vx_threshold threshold = (vx_threshold)parameters[1];
    int type = 0;
    vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));
    err = clSetKernelArg(kernel, argidx++, sizeof(vx_int32), &type);

    //Set value
    vx_pixel_value_t value;
    vx_pixel_value_t lower;
    vx_pixel_value_t upper;
    vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_VALUE, &value, sizeof(value));
    vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_LOWER, &lower, sizeof(lower));
    vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_UPPER, &upper, sizeof(upper));

#if 0

    err = clSetKernelArg(kernel, argidx++, sizeof(uint8_t), &value.U8);
    err = clSetKernelArg(kernel, argidx++, sizeof(uint8_t), &lower.U8);
    err = clSetKernelArg(kernel, argidx++, sizeof(uint8_t), &upper.U8);

#else

    err = clSetKernelArg(kernel, argidx++, sizeof(short), &value.S16);
    err = clSetKernelArg(kernel, argidx++, sizeof(short), &lower.S16);
    err = clSetKernelArg(kernel, argidx++, sizeof(short), &upper.S16);

#endif

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
    dir = node->kernel->signature.directions[0];
    if (dir == VX_INPUT || dir == VX_BIDIRECTIONAL)
    {
        memcpy(&writeEvents[we++], &ref->event, sizeof(cl_event));
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

vx_status vxThreshold_U1(vx_image src_image, vx_threshold threshold, vx_image dst_image)
{
    vx_enum type = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t src_addr;
    vx_imagepatch_addressing_t dst_addr;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_uint32 y = 0;
    vx_uint32 x = 0;
    vx_pixel_value_t value;
    vx_pixel_value_t lower;
    vx_pixel_value_t upper;
    vx_pixel_value_t true_value;
    vx_pixel_value_t false_value;
    vx_status status = VX_FAILURE;
    vx_enum in_format  = 0;
    vx_enum out_format = 0;

    vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));

    if (type == VX_THRESHOLD_TYPE_BINARY)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_VALUE, &value, sizeof(value));
        VX_PRINT(VX_ZONE_INFO, "threshold type binary, value = %u\n", value);
    }
    else if (type == VX_THRESHOLD_TYPE_RANGE)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_LOWER, &lower, sizeof(lower));
        vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_UPPER, &upper, sizeof(upper));
        VX_PRINT(VX_ZONE_INFO, "threshold type range, lower = %u, upper = %u\n", lower, upper);
    }

    vxQueryThreshold(threshold, VX_THRESHOLD_TRUE_VALUE, &true_value, sizeof(true_value));
    vxQueryThreshold(threshold, VX_THRESHOLD_FALSE_VALUE, &false_value, sizeof(false_value));
    VX_PRINT(VX_ZONE_INFO, "threshold true value = %u, threshold false value = %u\n", true_value, false_value);

    status = vxGetValidRegionImage(src_image, &rect);
    status |= vxAccessImagePatch(src_image, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);

    vxQueryThreshold(threshold, VX_THRESHOLD_INPUT_FORMAT,  &in_format,  sizeof(in_format));
    vxQueryThreshold(threshold, VX_THRESHOLD_OUTPUT_FORMAT, &out_format, sizeof(out_format));
    VX_PRINT(VX_ZONE_INFO, "threshold: input_format  = %d, output_format = %d\n", in_format, out_format);

    for (y = 0; y < src_addr.dim_y; y++)
    {
        for (x = 0; x < src_addr.dim_x; x++)
        {
            vx_uint32 xShftd = x + (out_format == VX_DF_IMAGE_U1 ? rect.start_x % 8 : 0);
            void *src_void_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
            void *dst_void_ptr = vxFormatImagePatchAddress2d(dst_base, xShftd, y, &dst_addr);

            if( in_format == VX_DF_IMAGE_S16 )
            {//case of input: VX_DF_IMAGE_S16

                vx_int16 *src_ptr = (vx_int16*)src_void_ptr;
                vx_uint8 *dst_ptr = (vx_uint8*)dst_void_ptr;

                vx_uint8 dst_value = 0;

                if (type == VX_THRESHOLD_TYPE_BINARY)
                {
                    if (*src_ptr > value.S16)
                        dst_value = true_value.U1  ? 1 : 0;
                    else
                        dst_value = false_value.U1 ? 1 : 0;

                    vx_uint8 offset = xShftd % 8;
                    *dst_ptr = (*dst_ptr & ~(1 << offset)) | (dst_value << offset);
                }

                if (type == VX_THRESHOLD_TYPE_RANGE)
                {
                    if (*src_ptr > upper.S16)
                        dst_value = false_value.U1 ? 1 : 0;
                    else if (*src_ptr < lower.S16)
                        dst_value = false_value.U1 ? 1 : 0;
                    else
                        dst_value = true_value.U1  ? 1 : 0;

                    vx_uint8 offset = xShftd % 8;
                    *dst_ptr = (*dst_ptr & ~(1 << offset)) | (dst_value << offset);
                }
            }
            else
            {//case of input: VX_DF_IMAGE_U8

                vx_uint8 *src_ptr = (vx_uint8*)src_void_ptr;
                vx_uint8 *dst_ptr = (vx_uint8*)dst_void_ptr;

                if (type == VX_THRESHOLD_TYPE_BINARY)
                {
                    vx_uint8 offset = xShftd % 8;
                    if (*src_ptr > value.U8)
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((true_value.U1  ? 1 : 0) << offset);
                    else
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value.U8 ? 1 : 0) << offset);
                }

                if (type == VX_THRESHOLD_TYPE_RANGE)
                {
                    vx_uint8 offset = xShftd % 8;
                    if (*src_ptr > upper.U8)
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value.U1 ? 1 : 0) << offset);
                    else if (*src_ptr < lower.U8)
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((false_value.U1 ? 1 : 0) << offset);
                    else
                        *dst_ptr = (*dst_ptr & ~(1 << offset)) | ((true_value.U1  ? 1 : 0) << offset);
                }
            } //end if else
        }//end for
    }//end for

    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &rect, 0, &dst_addr, dst_base);

    return status;
}


static vx_status VX_CALLBACK vxThresholdKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_image     src_image = (vx_image)    parameters[0];
    vx_threshold threshold = (vx_threshold)parameters[1];
    vx_image     dst_image = (vx_image)    parameters[2];
    vx_df_image out_format;
    vx_status status = vxQueryThreshold(threshold, VX_THRESHOLD_OUTPUT_FORMAT, &out_format, sizeof(out_format));
    if (status != VX_SUCCESS)
        return status;
    else if (out_format == VX_DF_IMAGE_U1)
        return vxThreshold_U1(src_image, threshold, dst_image);
    else
        return vxclCallOpenCLKernel(node, parameters, num);
}

static vx_status VX_CALLBACK vxThresholdInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (vxGetStatus((vx_reference)param) == VX_SUCCESS)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_REF, &input, sizeof(input));
            if (input)
            {
                vx_df_image format = 0;
                vxQueryImage(input, VX_IMAGE_FORMAT, &format, sizeof(format));
                if ((format == VX_DF_IMAGE_U8 || format == VX_DF_IMAGE_S16))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_FORMAT;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 1)
    {
        vx_parameter thr_param = vxGetParameterByIndex(node, index);
        vx_parameter img_param = vxGetParameterByIndex(node, 2);
        if ((vxGetStatus((vx_reference)thr_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)img_param) == VX_SUCCESS))
        {
            vx_threshold threshold = 0;
            vx_image     output = 0;
            vxQueryParameter(thr_param, VX_PARAMETER_REF, &threshold, sizeof(threshold));
            vxQueryParameter(img_param, VX_PARAMETER_REF, &output,    sizeof(output));
            if (threshold && output)
            {
                vx_enum type = 0;
                vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));
                if ((type == VX_THRESHOLD_TYPE_BINARY) ||
                    (type == VX_THRESHOLD_TYPE_RANGE))
                {
                    vx_enum     data_type = 0;
                    vx_df_image format = 0;
                    vxQueryThreshold(threshold, VX_THRESHOLD_DATA_TYPE, &data_type, sizeof(data_type));
                    vxQueryImage(output,        VX_IMAGE_FORMAT,        &format,    sizeof(format));
                    if (data_type == VX_TYPE_UINT8 && format == VX_DF_IMAGE_U8)
                    {
                        status = VX_SUCCESS;
                    }
                    else if (data_type == VX_TYPE_BOOL && format == VX_DF_IMAGE_U1)
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_TYPE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseThreshold(&threshold);
                vxReleaseImage(&output);
            }
            vxReleaseParameter(&thr_param);
            vxReleaseParameter(&img_param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxThresholdOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, 2);
        if ((vxGetStatus((vx_reference)src_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS))
        {
            vx_image src = 0, dst = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            if (src && dst)
            {
                vx_df_image in_format = 0, out_format = 0;
                vx_uint32 width = 0, height = 0;

                vxQueryImage(src, VX_IMAGE_WIDTH,  &width,      sizeof(height));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &height,     sizeof(height));
                vxQueryImage(src, VX_IMAGE_FORMAT, &in_format,  sizeof(in_format));
                vxQueryImage(dst, VX_IMAGE_FORMAT, &out_format, sizeof(out_format));

                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = out_format;
                ptr->dim.image.width  = width;
                ptr->dim.image.height = height;

                if ((out_format == VX_DF_IMAGE_U1 && in_format == VX_DF_IMAGE_U8) ||
                    (out_format == VX_DF_IMAGE_U8))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_FORMAT;
                }

                vxReleaseImage(&src);
                vxReleaseImage(&dst);
            }
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}


static vx_param_description_t threshold_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_THRESHOLD,   VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT,VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_cl_kernel_description_t threshold_clkernel = {
    {
    VX_KERNEL_THRESHOLD,
    "org.khronos.openvx.threshold",
    vxThresholdKernel,
    threshold_kernel_params, dimof(threshold_kernel_params),
    NULL,
    vxThresholdInputValidator,
    vxThresholdOutputValidator,
    NULL,
    NULL,
    },
    VX_CL_SOURCE_DIR""FILE_JOINER"vx_threshold.cl",
    "vx_threshold",
    INIT_PROGRAMS,
    INIT_KERNELS,
    INIT_NUMKERNELS,
    INIT_RETURNS,
    NULL,
};
