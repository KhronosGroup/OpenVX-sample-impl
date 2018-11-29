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
 * \brief Edge tracing for the Canny edge detector.
 */

#include <math.h>
#include <stdlib.h>
#include <VX/vx.h>
/* For VX_THRESHOLD_THRESHOLD_LOWER and VX_THRESHOLD_THRESHOLD_UPPER: */
#include <VX/vx_compatibility.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>

static const struct offset_t
{
    int x;
    int y;
} dir_offsets[8] = {
    {  0, -1 },
    { -1, -1 },
    { -1,  0 },
    { -1, +1 },
    {  0, +1 },
    { +1, +1 },
    { +1,  0 },
    { +1, -1 },
};

static
vx_status ownEdgeTrace(vx_image norm, vx_threshold threshold, vx_image output)
{
    vx_uint32 x = 0;
    vx_uint32 y = 0;
    vx_int32 lower = 0;
    vx_int32 upper = 0;
    void* norm_base = NULL;
    void* output_base = NULL;
    vx_imagepatch_addressing_t norm_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_imagepatch_addressing_t output_addr = VX_IMAGEPATCH_ADDR_INIT;
    vx_map_id norm_map_id = 0;
    vx_map_id output_map_id = 0;
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;

    status |= vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_LOWER, &lower, sizeof(lower));
    status |= vxQueryThreshold(threshold, VX_THRESHOLD_THRESHOLD_UPPER, &upper, sizeof(upper));

    status |= vxGetValidRegionImage(norm, &rect);

    status |= vxMapImagePatch(norm, &rect, 0, &norm_map_id, &norm_addr, &norm_base, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);
    status |= vxMapImagePatch(output, &rect, 0, &output_map_id, &output_addr, &output_base, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST, VX_NOGAP_X);

    if (status == VX_SUCCESS)
    {
        const vx_uint8 NO    = 0;
        const vx_uint8 MAYBE = 127;
        const vx_uint8 YES   = 255;

        /* Initially we add all YES pixels to the stack. Later we only add MAYBE
           pixels to it, and we reset their state to YES afterwards; so we can never
           add the same pixel more than once. That means that the stack size is bounded
           by the image size. */
        vx_uint32(*tracing_stack)[2] = malloc(output_addr.dim_y * output_addr.dim_x * sizeof *tracing_stack);
        vx_uint32(*stack_top)[2] = tracing_stack;

        if (NULL == tracing_stack)
            return VX_ERROR_NO_MEMORY;

        for (y = 0; y < norm_addr.dim_y; y++)
        {
            for (x = 0; x < norm_addr.dim_x; x++)
            {
                vx_float32* norm_ptr = vxFormatImagePatchAddress2d(norm_base, x, y, &norm_addr);
                vx_uint8* output_ptr = vxFormatImagePatchAddress2d(output_base, x, y, &output_addr);

                if (*norm_ptr > upper)
                {
                    *output_ptr = YES;
                    (*stack_top)[0] = x;
                    (*stack_top)[1] = y;
                    ++stack_top;
                }
                else if (*norm_ptr <= lower)
                {
                    *output_ptr = NO;
                }
                else
                {
                    *output_ptr = MAYBE;
                }
            }
        }

        while (stack_top != tracing_stack)
        {
            int i;
            --stack_top;
            x = (*stack_top)[0];
            y = (*stack_top)[1];

            for (i = 0; i < (int)dimof(dir_offsets); ++i)
            {
                const struct offset_t offset = dir_offsets[i];
                vx_uint32 new_x;
                vx_uint32 new_y;
                vx_uint8* output_ptr;

                if (x == 0 && offset.x < 0)
                    continue;

                if (x == output_addr.dim_x - 1 && offset.x > 0)
                    continue;

                if (y == 0 && offset.y < 0)
                    continue;

                if (y == output_addr.dim_y - 1 && offset.y > 0)
                    continue;

                new_x = x + offset.x;
                new_y = y + offset.y;

                output_ptr = vxFormatImagePatchAddress2d(output_base, new_x, new_y, &output_addr);
                if (*output_ptr != MAYBE)
                    continue;

                *output_ptr = YES;

                (*stack_top)[0] = new_x;
                (*stack_top)[1] = new_y;
                ++stack_top;
            }
        }

        free(tracing_stack);

        for (y = 0; y < output_addr.dim_y; y++)
        {
            for (x = 0; x < output_addr.dim_x; x++)
            {
                vx_uint8 *output_ptr = vxFormatImagePatchAddress2d(output_base, x, y, &output_addr);
                if (*output_ptr == MAYBE) *output_ptr = NO;
            }
        }

        status |= vxUnmapImagePatch(norm, norm_map_id);
        status |= vxUnmapImagePatch(output, output_map_id);
    }

    return status;
} /* ownEdgeTrace() */


/*
// Edge trace kernel implementation
*/

static
vx_param_description_t edge_trace_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,     VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_THRESHOLD, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,     VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownEdgeTraceKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(edge_trace_kernel_params))
    {
        vx_image     norm      = (vx_image)parameters[0];
        vx_threshold threshold = (vx_threshold)parameters[1];
        vx_image     output    = (vx_image)parameters[2];

        status = ownEdgeTrace(norm, threshold, output);
    }

    return status;
} /* ownEdgeTraceKernel() */

static
vx_status VX_CALLBACK set_edge_trace_valid_rectangle(
    vx_node node,
    vx_uint32 index,
    const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && index < dimof(edge_trace_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
        output_valid[0]->start_x = input_valid[0]->start_x;
        output_valid[0]->start_y = input_valid[0]->start_y;
        output_valid[0]->end_x   = input_valid[0]->end_x;
        output_valid[0]->end_y   = input_valid[0]->end_y;
        status = VX_SUCCESS;
    }

    return status;
} /* set_edge_trace_valid_rectangle() */

static
vx_status VX_CALLBACK own_edge_trace_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (NULL != node &&
        num == dimof(edge_trace_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;

        vx_image norm = 0;
        vx_threshold threshold = 0;

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2))
        {
            /* validate input image */
            status = vxQueryParameter(param1, VX_PARAMETER_REF, &norm, sizeof(norm));
            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)norm))
            {
                vx_df_image format = 0;
                status |= vxQueryImage(norm, VX_IMAGE_FORMAT, &format, sizeof(format));
                if (format == VX_DF_IMAGE_F32)
                    status = VX_SUCCESS;
                else
                    status = VX_ERROR_INVALID_FORMAT;
            }

            /* validate input threshold */
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &threshold, sizeof(threshold));
            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)threshold))
            {
                vx_enum type = 0;
                status |= vxQueryThreshold(threshold, VX_THRESHOLD_TYPE, &type, sizeof(type));
                if (type == VX_THRESHOLD_TYPE_RANGE)
                    status = VX_SUCCESS;
                else
                    status = VX_ERROR_INVALID_TYPE;
            }

            /* validate output image */
            if (VX_SUCCESS == status)
            {
                vx_uint32   src_width  = 0;
                vx_uint32   src_height = 0;
                vx_df_image dst_format = VX_DF_IMAGE_U8;
                vx_kernel_image_valid_rectangle_f callback = &set_edge_trace_valid_rectangle;

                status |= vxQueryImage(norm, VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                status |= vxQueryImage(norm, VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));

                status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_FORMAT, &dst_format, sizeof(dst_format));

                status |= vxSetMetaFormatAttribute(metas[2], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
            }
        }

        if (NULL != norm)
            vxReleaseImage(&norm);

        if (NULL != threshold)
            vxReleaseThreshold(&threshold);

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);
    } // if ptrs non NULL

    return status;
} /* own_edge_trace_validator() */

vx_kernel_description_t edge_trace_kernel =
{
    VX_KERNEL_EXTRAS_EDGE_TRACE,
    "org.khronos.extra.edge_trace",
    ownEdgeTraceKernel,
    edge_trace_kernel_params, dimof(edge_trace_kernel_params),
    own_edge_trace_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
