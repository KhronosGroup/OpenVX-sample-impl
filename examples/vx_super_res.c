/*

 * Copyright (c) 2013-2017 The Khronos Group Inc.
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
 * \file vx_super_res.c
 * \brief The example graph used to several images into a super resolution output.
 * \author Tomer Schwartz <tomer@broadcom.com>
 */

#include <vx_examples.h>

void userCalculatePerspectiveTransformFromLK(vx_matrix matrix_forward, vx_matrix matrix_backward, vx_array old_features,
        vx_array new_features)
{
    vx_float32 mat1[3][3];
    vx_float32 mat2[3][3];
    vx_size olen = 0;
    vx_size nlen = 0;
    vx_keypoint_t *old_features_ptr = NULL;
    vx_keypoint_t *new_features_ptr = NULL;
    vx_size old_features_stride = 0;
    vx_size new_features_stride = 0;
    vx_uint32 ind;
    vx_map_id map_id_old, map_id_new;
    //vx_float32 *A = malloc(olen+nlen);
    //vx_float32 *b = malloc(olen+nlen);

    vxQueryArray(old_features, VX_ARRAY_NUMITEMS, &olen, sizeof(olen));
    vxQueryArray(new_features, VX_ARRAY_NUMITEMS, &nlen, sizeof(nlen));

    if (olen != nlen)
        return;

    vxMapArrayRange(old_features, 0, olen, &map_id_old, &old_features_stride, (void **) &old_features_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
    vxMapArrayRange(new_features, 0, nlen, &map_id_new, &new_features_stride, (void **) &new_features_ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

    /*! \internal do least square algorithm that find perspective transform */
    /*! \see "Computer Vision Algorithm and Application by Richard Szeliski section 6.1.3 */
    for (ind = 0; ind < nlen; ind++)
    {
        //vx_int32 matrix_index = 2 * ind;

        //vx_keypoint_t old_kp = vxArrayItem(vx_keypoint_t, old_features_ptr, ind, old_features_stride);
        //vx_keypoint_t new_kp = vxArrayItem(vx_keypoint_t, new_features_ptr, ind, new_features_stride);

        /*
         A[matrix_index] = {old_kp.x, old_kp.y,    1,
         0,       0,    0,
         -old_kp.x*new_kp.x,
         -old_kp.x*new_kp.y};

         A[matrix_index+1] = {0, 0, 0,
         old_kp.x,
         old_kp.y,
         1,
         -old_kp.y*new_kp.x,
         -old_kp.y*new_kp.y};

         b[matrix_index]   = new_kp.x - old_kp.x;
         b[matrix_index+1] = new_kp.y - old_kp.y;
         */
    }

    //least_square_divide(A,b, mat1);
    //inverse_matrix(mat1, mat2);

    vxUnmapArrayRange(old_features, map_id_old);
    vxUnmapArrayRange(new_features, map_id_new);

    vxCopyMatrix(matrix_forward, mat1, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    vxCopyMatrix(matrix_backward, mat2, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
}

/*!
 * \brief An example of an super resolution algorithm.
 * \ingroup group_example
 */
int example_super_resolution(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    vx_status status = VX_SUCCESS;
    vx_uint32 image_index = 0, max_num_images = 4;
    vx_uint32 width = 640;
    vx_uint32 i = 0;
    vx_uint32 winSize = 32;
    vx_uint32 height = 480;
    vx_int32 sens_thresh = 20;
    vx_float32 alpha = 0.2f;
    vx_float32 tau = 0.5f;
    vx_enum criteria = VX_TERM_CRITERIA_BOTH;    // lk params
    vx_float32 epsilon = 0.01f;
    vx_int32 num_iterations = 10;
    vx_bool use_initial_estimate = vx_true_e;
    vx_int32 min_distance = 5;    // harris params
    vx_float32 sensitivity = 0.04f;
    vx_int32 gradient_size = 3;
    vx_int32 block_size = 3;
    vx_context context = vxCreateContext();
    vx_scalar alpha_s = vxCreateScalar(context, VX_TYPE_FLOAT32, &alpha);
    vx_scalar tau_s = vxCreateScalar(context, VX_TYPE_FLOAT32, &tau);
    vx_matrix matrix_forward = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3);
    vx_matrix matrix_backwords = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3);
    vx_array old_features = vxCreateArray(context, VX_TYPE_KEYPOINT, 1000);
    vx_array new_features = vxCreateArray(context, VX_TYPE_KEYPOINT, 1000);
    vx_scalar epsilon_s = vxCreateScalar(context, VX_TYPE_FLOAT32, &epsilon);
    vx_scalar num_iterations_s = vxCreateScalar(context, VX_TYPE_INT32, &num_iterations);
    vx_scalar use_initial_estimate_s = vxCreateScalar(context, VX_TYPE_BOOL, &use_initial_estimate);
    vx_scalar min_distance_s = vxCreateScalar(context, VX_TYPE_INT32, &min_distance);
    vx_scalar sensitivity_s = vxCreateScalar(context, VX_TYPE_FLOAT32, &sensitivity);
    vx_scalar sens_thresh_s = vxCreateScalar(context, VX_TYPE_INT32, &sens_thresh);
    vx_scalar num_corners = vxCreateScalar(context, VX_TYPE_SIZE, NULL);

    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_image images[] =
        { vxCreateImage(context, width, height, VX_DF_IMAGE_UYVY),     // index 0:
        vxCreateImage(context, width, height, VX_DF_IMAGE_U8),       // index 1: Get Y channel
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8),   // index 2: scale up to high res.
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8), // index 3: back wrap: transform to the original Image.
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8),   // index 4: guassian blur
        vxCreateImage(context, width, height, VX_DF_IMAGE_U8),       // index 5: scale down
        vxCreateImage(context, width, height, VX_DF_IMAGE_S16), // index 6: Subtract the transformed Image with original moved Image
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_S16),  // index 7: Scale Up the delta image.
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_S16),  // index 8: Guassian blur the delta Image
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_S16), // index 9: forward wrap: tranform the deltas back to the high res Image
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8),    // index 10: accumulate sum?
        vxCreateImage(context, width, height, VX_DF_IMAGE_U8),       // index 11: Get U channel
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8),   // index 12: scale up to high res.
        vxCreateImage(context, width, height, VX_DF_IMAGE_U8),       // index 13: Get V channel
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8),   // index 14: scale up to high res.
        vxCreateImage(context, width, height, VX_DF_IMAGE_UYVY),     // index 15: output image
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8),   // index 16: original y image scaled
        vxCreateImage(context, width * 2, height * 2, VX_DF_IMAGE_U8),   // index 17: difference image for last calculation
                };
        vx_pyramid pyramid_new = vxCreatePyramid(context, 4, 2, width, height, VX_DF_IMAGE_U8);
        vx_pyramid pyramid_old = vxCreatePyramid(context, 4, 2, width, height, VX_DF_IMAGE_U8);

        vx_graph graphs[] =
        { vxCreateGraph(context), vxCreateGraph(context), vxCreateGraph(context), vxCreateGraph(context), };
        vxLoadKernels(context, "openvx-debug");
        if (vxGetStatus((vx_reference)graphs[0]) == VX_SUCCESS)
        {
            vxChannelExtractNode(graphs[0], images[0], VX_CHANNEL_Y, images[1]); // One iteration of super resolution calculation
            vxScaleImageNode(graphs[0], images[1], images[2], VX_INTERPOLATION_BILINEAR);
            vxWarpPerspectiveNode(graphs[0], images[2], matrix_forward, 0, images[3]);
            vxGaussian3x3Node(graphs[0], images[3], images[4]);
            vxScaleImageNode(graphs[0], images[4], images[5], VX_INTERPOLATION_BILINEAR);
            vxSubtractNode(graphs[0], images[5], images[16], VX_CONVERT_POLICY_SATURATE, images[6]);
            vxScaleImageNode(graphs[0], images[6], images[7], VX_INTERPOLATION_BILINEAR);
            vxGaussian3x3Node(graphs[0], images[7], images[8]);
            vxWarpPerspectiveNode(graphs[0], images[8], matrix_backwords, 0, images[9]);
            vxAccumulateWeightedImageNode(graphs[0], images[9], alpha_s, images[10]);

        }
        if (vxGetStatus((vx_reference)graphs[1]) == VX_SUCCESS)
        {
            vxChannelExtractNode(graphs[1], images[0], VX_CHANNEL_Y, images[1]); // One iteration of super resolution calculation
            vxGaussianPyramidNode(graphs[1], images[1], pyramid_new);

            vxOpticalFlowPyrLKNode(graphs[1], pyramid_old, pyramid_new, old_features, old_features, new_features,
                    criteria, epsilon_s, num_iterations_s, use_initial_estimate_s, winSize);
        }
        if (vxGetStatus((vx_reference)graphs[2]) == VX_SUCCESS)
        {
            vxChannelExtractNode(graphs[2], images[0], VX_CHANNEL_Y, images[1]); // One iteration of super resolution calculation

            vxHarrisCornersNode(graphs[2], images[1], sens_thresh_s, min_distance_s, sensitivity_s, gradient_size,
                    block_size, old_features, num_corners);
            vxGaussianPyramidNode(graphs[2], images[1], pyramid_old);
            vxScaleImageNode(graphs[2], images[1], images[16], VX_INTERPOLATION_BILINEAR);
        }
        if (vxGetStatus((vx_reference)graphs[3]) == VX_SUCCESS)
        {
            vxSubtractNode(graphs[3], images[10], images[16], VX_CONVERT_POLICY_SATURATE, images[17]);
            vxAccumulateWeightedImageNode(graphs[3], images[17], tau_s, images[16]);
            vxChannelExtractNode(graphs[3], images[16], VX_CHANNEL_U, images[11]);
            vxScaleImageNode(graphs[3], images[11], images[12], VX_INTERPOLATION_BILINEAR); // upscale the u channel
            vxChannelExtractNode(graphs[3], images[0], VX_CHANNEL_V, images[13]);
            vxScaleImageNode(graphs[3], images[13], images[14], VX_INTERPOLATION_BILINEAR); // upscale the v channel
            vxChannelCombineNode(graphs[3], images[10], images[12], images[14], 0, images[15]); // recombine the channels

        }

        status = VX_SUCCESS;
        status |= vxVerifyGraph(graphs[0]);
        status |= vxVerifyGraph(graphs[1]);
        status |= vxVerifyGraph(graphs[2]);
        status |= vxVerifyGraph(graphs[3]);
        if (status == VX_SUCCESS)
        {
            /* read the initial image in */
            status |= vxuFReadImage(context, "c:\\work\\super_res\\superres_1_UYVY.yuv", images[0]);
            /* compute the "old" pyramid */
            status |= vxProcessGraph(graphs[2]);

            /* for each input image, read it in and run graphs[1] and [0]. */
            for (image_index = 1; image_index < max_num_images; image_index++)
            {
                char filename[256];

                sprintf(filename, "c:\\work\\super_res\\superres_%d_UYVY.yuv", image_index + 1);
                status |= vxuFReadImage(context, filename, images[0]);
                status |= vxProcessGraph(graphs[1]);
                userCalculatePerspectiveTransformFromLK(matrix_forward, matrix_backwords, old_features, new_features);
                status |= vxProcessGraph(graphs[0]);
            }
            /* run the final graph */
            status |= vxProcessGraph(graphs[3]);
            /* save the output */
            status |= vxuFWriteImage(context, images[15], "superres_UYVY.yuv");
        }
        vxReleaseGraph(&graphs[0]);
        vxReleaseGraph(&graphs[1]);
        vxReleaseGraph(&graphs[2]);
        vxReleaseGraph(&graphs[3]);
        for (i = 0; i < dimof(images); i++)
        {
            vxReleaseImage(&images[i]);
        }
        vxReleasePyramid(&pyramid_new);
        vxReleasePyramid(&pyramid_old);

        vxUnloadKernels(context, "openvx-debug");
    }
    vxReleaseMatrix(&matrix_forward);
    vxReleaseMatrix(&matrix_backwords);
    vxReleaseScalar(&alpha_s);
    vxReleaseScalar(&tau_s);
    /* Release the context last */
    vxReleaseContext(&context);
    return status;
}

