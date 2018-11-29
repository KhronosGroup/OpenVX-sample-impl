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
 * \brief The Non-Maxima Suppression Kernel (Extras)
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <math.h>
#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>
#include "extras_k.h"

#ifndef OWN_MAX
#define OWN_MAX(a, b) (a) > (b) ? (a) : (b)
#endif

#ifndef OWN_MIN
#define OWN_MIN(a, b) (a) < (b) ? (a) : (b)
#endif

/* euclidian nonmaxsuppression kernel implementation */
static
vx_param_description_t euclidean_nonmaxsuppression_harris_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED }, /* strength_thresh */
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED }, /* min_distance */
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownEuclideanNonMaxSuppressionHarrisKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(euclidean_nonmaxsuppression_harris_kernel_params))
    {
        vx_image  src = (vx_image) parameters[0];
        vx_scalar thr = (vx_scalar)parameters[1];
        vx_scalar rad = (vx_scalar)parameters[2];
        vx_image  dst = (vx_image) parameters[3];

        status = ownEuclideanNonMaxSuppressionHarris(src, thr, rad, dst);
    }

    return status;
} /* ownEuclideanNonMaxSuppressionHarrisKernel() */

static
vx_status VX_CALLBACK set_euclidean_nonmaxsuppression_harris_valid_rectangle(
    vx_node node,
    vx_uint32 index,
    const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && index < dimof(euclidean_nonmaxsuppression_harris_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
        output_valid[0]->start_x = input_valid[0]->start_x;
        output_valid[0]->start_y = input_valid[0]->start_y;
        output_valid[0]->end_x   = input_valid[0]->end_x;
        output_valid[0]->end_y   = input_valid[0]->end_y;
        status = VX_SUCCESS;
    }

    return status;
} /* set_euclidean_nonmaxsuppression_harris_valid_rectangle() */

static
vx_status VX_CALLBACK own_euclidean_nonmaxsuppresson_harris_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (NULL != node &&
        num == dimof(euclidean_nonmaxsuppression_harris_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;
        vx_parameter param3 = 0;

        vx_image     src = 0;
        vx_df_image  src_format = 0;
        vx_uint32    src_width = 0;
        vx_uint32    src_height = 0;
        vx_scalar    strength_threshold = 0;
        vx_scalar    min_distance = 0;

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);
        param3 = vxGetParameterByIndex(node, 2);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param3))
        {
            /* validate input image */
            status = vxQueryParameter(param1, VX_PARAMETER_REF, &src, sizeof(src));
            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)src))
            {
                status |= vxQueryImage(src, VX_IMAGE_WIDTH,  &src_width,  sizeof(src_width));
                status |= vxQueryImage(src, VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                status |= vxQueryImage(src, VX_IMAGE_FORMAT, &src_format, sizeof(src_format));

                if (VX_SUCCESS == status && VX_DF_IMAGE_F32 == src_format)
                    status = VX_SUCCESS;
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            /* validate input strength_threshold */
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &strength_threshold, sizeof(strength_threshold));
            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)strength_threshold))
            {
                vx_enum type = 0;
                status |= vxQueryScalar(strength_threshold, VX_SCALAR_TYPE, &type, sizeof(type));
                if (VX_SUCCESS == status && VX_TYPE_FLOAT32 == type)
                    status = VX_SUCCESS;
                else
                    status = VX_ERROR_INVALID_TYPE;
            }

            /* validate input min_distance */
            status |= vxQueryParameter(param3, VX_PARAMETER_REF, &min_distance, sizeof(min_distance));
            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)min_distance))
            {
                vx_enum type = 0;
                status |= vxQueryScalar(min_distance, VX_SCALAR_TYPE, &type, sizeof(type));
                if (VX_SUCCESS == status && VX_TYPE_FLOAT32 == type)
                {
                    vx_float32 radius = 0;
                    status |= vxCopyScalar(min_distance, &radius, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if (VX_SUCCESS == status &&
                        (0.0 <= radius) && (radius <= 30.0))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                        status = VX_ERROR_INVALID_PARAMETERS;
                }
                else
                    status = VX_ERROR_INVALID_TYPE;
            }

            /* validate output image */
            if (VX_SUCCESS == status)
            {
                vx_kernel_image_valid_rectangle_f callback = &set_euclidean_nonmaxsuppression_harris_valid_rectangle;

                status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_WIDTH, &src_width, sizeof(src_width));
                status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_HEIGHT, &src_height, sizeof(src_height));
                status |= vxSetMetaFormatAttribute(metas[3], VX_IMAGE_FORMAT, &src_format, sizeof(src_format));

                status |= vxSetMetaFormatAttribute(metas[3], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
            }
        }

        if (NULL != src)
            vxReleaseImage(&src);

        if (NULL != strength_threshold)
            vxReleaseScalar(&strength_threshold);

        if (NULL != min_distance)
            vxReleaseScalar(&min_distance);

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);

        if (NULL != param3)
            vxReleaseParameter(&param3);
    } // if ptrs non NULL

    return status;
} /* own_euclidean_nonmaxsuppresson_harris_validator() */

vx_kernel_description_t euclidean_nonmaxsuppression_harris_kernel =
{
    VX_KERNEL_EXTRAS_EUCLIDEAN_NONMAXSUPPRESSION_HARRIS,
    "org.khronos.extra.euclidean_nonmaxsuppression_harris",
    ownEuclideanNonMaxSuppressionHarrisKernel,
    euclidean_nonmaxsuppression_harris_kernel_params, dimof(euclidean_nonmaxsuppression_harris_kernel_params),
    own_euclidean_nonmaxsuppresson_harris_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};


/* NonMaxSuppression kernel implementation */

static
vx_param_description_t nonmaxsuppression_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED },
};

static
vx_status VX_CALLBACK ownNonMaxSuppressionKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && NULL != parameters && num == dimof(nonmaxsuppression_kernel_params))
    {
        vx_image i_mag  = (vx_image)parameters[0];
        vx_image i_ang  = (vx_image)parameters[1];
        vx_image i_edge = (vx_image)parameters[2];

        vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };

        status = vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));

        status |= ownNonMaxSuppression(i_mag, i_ang, i_edge, &borders);
    }

    return status;
} /* ownNonMaxSuppressionKernel() */

static
vx_status VX_CALLBACK set_nonmaxsuppression_valid_rectangle(
    vx_node node, vx_uint32 index, const vx_rectangle_t* const input_valid[], vx_rectangle_t* const output_valid[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node && index <= dimof(nonmaxsuppression_kernel_params) && NULL != input_valid && NULL != output_valid)
    {
        vx_border_t borders = { VX_BORDER_UNDEFINED, { { 0 } } };
        vx_uint32   border_size = 0;

        status = vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
        if (VX_SUCCESS != status)
            return status;

        if (VX_BORDER_UNDEFINED == borders.mode)
        {
            border_size = 1;
        }

        if (input_valid[0]->start_x > input_valid[1]->end_x ||
            input_valid[0]->end_x   < input_valid[1]->start_x ||
            input_valid[0]->start_y > input_valid[1]->end_y ||
            input_valid[0]->end_y   < input_valid[1]->start_y)
        {
            /* no intersection */
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        else
        {
            output_valid[0]->start_x = OWN_MAX(input_valid[0]->start_x, input_valid[1]->start_x) + border_size;
            output_valid[0]->start_y = OWN_MAX(input_valid[0]->start_y, input_valid[1]->start_y) + border_size;
            output_valid[0]->end_x   = OWN_MIN(input_valid[0]->end_x, input_valid[1]->end_x) - border_size;
            output_valid[0]->end_y   = OWN_MIN(input_valid[0]->end_y, input_valid[1]->end_y) - border_size;
            status = VX_SUCCESS;
        }
    }

    return status;
} /* set_nonmaxsuppression_valid_rectangle() */

static
vx_status VX_CALLBACK own_nonmaxsuppresson_validator(
    vx_node node,
    const vx_reference parameters[], vx_uint32 num, vx_meta_format metas[])
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (NULL != node &&
        NULL != parameters &&
        num == dimof(nonmaxsuppression_kernel_params) &&
        NULL != metas)
    {
        vx_parameter param1 = 0;
        vx_parameter param2 = 0;

        vx_image     mag = 0;
        vx_df_image  mag_format = 0;
        vx_uint32    mag_width = 0;
        vx_uint32    mag_height = 0;
        vx_image     ang = 0;
        vx_df_image  ang_format = 0;
        vx_uint32    ang_width = 0;
        vx_uint32    ang_height = 0;

        param1 = vxGetParameterByIndex(node, 0);
        param2 = vxGetParameterByIndex(node, 1);

        if (VX_SUCCESS == vxGetStatus((vx_reference)param1) &&
            VX_SUCCESS == vxGetStatus((vx_reference)param2))
        {
            status = VX_SUCCESS;

            /* validate input images */
            status |= vxQueryParameter(param1, VX_PARAMETER_REF, &mag, sizeof(mag));
            status |= vxQueryParameter(param2, VX_PARAMETER_REF, &ang, sizeof(ang));

            if (VX_SUCCESS == status &&
                VX_SUCCESS == vxGetStatus((vx_reference)mag) &&
                VX_SUCCESS == vxGetStatus((vx_reference)ang))
            {
                status |= vxQueryImage(mag, VX_IMAGE_WIDTH,  &mag_width,  sizeof(mag_width));
                status |= vxQueryImage(mag, VX_IMAGE_HEIGHT, &mag_height, sizeof(mag_height));
                status |= vxQueryImage(mag, VX_IMAGE_FORMAT, &mag_format, sizeof(mag_format));

                status |= vxQueryImage(ang, VX_IMAGE_WIDTH,  &ang_width,  sizeof(ang_width));
                status |= vxQueryImage(ang, VX_IMAGE_HEIGHT, &ang_height, sizeof(ang_height));
                status |= vxQueryImage(ang, VX_IMAGE_FORMAT, &ang_format, sizeof(ang_format));

                if (VX_SUCCESS == status &&
                    ((VX_DF_IMAGE_U8 == mag_format) || (VX_DF_IMAGE_S16 == mag_format) || (VX_DF_IMAGE_U16 == mag_format) || (VX_DF_IMAGE_F32 == mag_format)) &&
                    VX_DF_IMAGE_U8 == ang_format &&
                    mag_width == ang_width &&
                    mag_height == ang_height)
                {
                    status = VX_SUCCESS;
                }
                else
                    status = VX_ERROR_INVALID_PARAMETERS;
            }

            /* validate output image */
            if (VX_SUCCESS == status)
            {
                vx_kernel_image_valid_rectangle_f callback = &set_nonmaxsuppression_valid_rectangle;

                status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_WIDTH, &mag_width, sizeof(mag_width));
                status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_HEIGHT, &mag_height, sizeof(mag_height));
                status |= vxSetMetaFormatAttribute(metas[2], VX_IMAGE_FORMAT, &mag_format, sizeof(mag_format));

                status |= vxSetMetaFormatAttribute(metas[2], VX_VALID_RECT_CALLBACK, &callback, sizeof(callback));
            }
        }

        if (NULL != mag)
            vxReleaseImage(&mag);

        if (NULL != ang)
            vxReleaseImage(&ang);

        if (NULL != param1)
            vxReleaseParameter(&param1);

        if (NULL != param2)
            vxReleaseParameter(&param2);
    } // if ptrs non NULL

    return status;
} /* own_nonmaxsuppresson_validator() */

vx_kernel_description_t nonmaxsuppression_kernel =
{
    VX_KERNEL_EXTRAS_NONMAXSUPPRESSION_CANNY,
    "org.khronos.extra.nonmaximasuppression",
    ownNonMaxSuppressionKernel,
    nonmaxsuppression_kernel_params, dimof(nonmaxsuppression_kernel_params),
    own_nonmaxsuppresson_validator,
    NULL,
    NULL,
    NULL,
    NULL,
};
