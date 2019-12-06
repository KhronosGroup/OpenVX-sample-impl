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
 * \brief The Image Scale Kernel
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>

#include "vx_internal.h"
#include <c_model.h>
#include <stdio.h>
#include <math.h>

/* Specification 1.0, on AREA interpolation method: "The details of this
 * sampling method are implementation defined. The implementation should
 * perform enough sampling to avoid aliasing, but there is no requirement
 * that the sample areas for adjacent output pixels be disjoint, nor that
 * the pixels be weighted evenly."
 *
 * The existing implementation of AREA can use too much heap in certain
 * circumstances, so disabling for now, and using NEAREST_NEIGHBOR instead,
 * as it also passes conformance for AREA interpolation.
 */
#define AREA_SCALE_ENABLE 0 /* TODO enable this again after changing implementation in kernels/c_model/c_scale.c */

/* scale image kernel */
static vx_param_description_t scale_kernel_params[] =
{
    { VX_INPUT,  VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_OUTPUT, VX_TYPE_IMAGE,  VX_PARAMETER_STATE_REQUIRED },
    { VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL },
};

static vx_status VX_CALLBACK vxScaleImageKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == dimof(scale_kernel_params))
    {
        vx_image  src_image = (vx_image)parameters[0];
        vx_image  dst_image = (vx_image)parameters[1];
        vx_scalar stype     = (vx_scalar)parameters[2];
        vx_border_t bordermode = { VX_BORDER_UNDEFINED, {{ 0 }} };
        vx_float64 *interm = NULL;
        vx_size size = 0ul;

        vxQueryNode(node, VX_NODE_BORDER, &bordermode, sizeof(bordermode));
        vxQueryNode(node, VX_NODE_LOCAL_DATA_PTR, &interm, sizeof(interm));
        vxQueryNode(node, VX_NODE_LOCAL_DATA_SIZE, &size, sizeof(size));

        return vxScaleImage(src_image, dst_image, stype, &bordermode, interm, size);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxScaleImageInputValidator(vx_node node, vx_uint32 index)
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

            if (format == VX_DF_IMAGE_U1 || format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            else if (format == VX_DF_IMAGE_S16)
            {
                /* enable internal S16 format support (needed for laplacian pyramid reconstruction) */
                vx_scalar scalar = 0;
                vx_parameter param1 = vxGetParameterByIndex(node, 2);
                vxQueryParameter(param1, VX_PARAMETER_REF, &scalar, sizeof(scalar));
                if (scalar)
                {
                    vx_enum stype = 0;
                    vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype, sizeof(stype));
                    if (VX_TYPE_ENUM == stype)
                    {
                        vx_enum interp = 0;
                        vxCopyScalar(scalar, &interp, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                        if (VX_INTERPOLATION_NEAREST_NEIGHBOR == interp)
                        {
                            /* only NN interpolation is required for laplacian pyramid */
                            status = VX_SUCCESS;
                        }
                    }

                    vxReleaseScalar(&scalar);
                }

                vxReleaseParameter(&param1);
            }

            vxReleaseImage(&input);
        }

        vxReleaseParameter(&param);
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
                        (interp == VX_INTERPOLATION_BILINEAR) ||
                        (interp == VX_INTERPOLATION_AREA))
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

static vx_status VX_CALLBACK vxScaleImageOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if ((vxGetStatus((vx_reference)src_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS))
        {
            vx_image src = 0;
            vx_image dst = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            if ((src) && (dst))
            {
                vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
                vx_df_image f1 = VX_DF_IMAGE_VIRT, f2 = VX_DF_IMAGE_VIRT;

                vxQueryImage(src, VX_IMAGE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(dst, VX_IMAGE_WIDTH, &w2, sizeof(w2));
                vxQueryImage(dst, VX_IMAGE_HEIGHT, &h2, sizeof(h2));
                vxQueryImage(src, VX_IMAGE_FORMAT, &f1, sizeof(f1));
                vxQueryImage(dst, VX_IMAGE_FORMAT, &f2, sizeof(f2));
                /* output can not be virtual */
                if ((w2 != 0) && (h2 != 0) && (f2 != VX_DF_IMAGE_VIRT) && (f1 == f2))
                {
                    /* fill in the meta data with the attributes so that the checker will pass */
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = f2;
                    ptr->dim.image.width = w2;
                    ptr->dim.image.height = h2;
                    status = VX_SUCCESS;
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

static vx_status VX_CALLBACK vxScaleImageInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == dimof(scale_kernel_params))
    {
        vx_image src = (vx_image)parameters[0];
        vx_image dst = (vx_image)parameters[1];
        vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
#if AREA_SCALE_ENABLE
        vx_uint32 gcd_w = 0, gcd_h = 0;
#endif
        vx_size size = 0;
        vx_size kernel_data_size = 0;

        vxQueryImage(src, VX_IMAGE_WIDTH, &w1, sizeof(w1));
        vxQueryImage(src, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
        vxQueryImage(dst, VX_IMAGE_WIDTH, &w2, sizeof(w2));
        vxQueryImage(dst, VX_IMAGE_HEIGHT, &h2, sizeof(h2));

        /* AREA interpolation requires a scratch buffer, however, if AREA
        * implementation is disabled, then no scratch buffer is required, and
        * size can be 0 (setting to 1 so that checks can pass in the kernel) */
#if AREA_SCALE_ENABLE
        gcd_w = math_gcd(w1, w2);
        gcd_h = math_gcd(h1, h2);
        /* printf("%ux%u => %ux%u :: GCD_w %u GCD_h %u\n", w1,h1, w2,h2, gcd_w, gcd_h); */
        if (gcd_w != 0 && gcd_h != 0)
        {
            size = (w1 / gcd_w) * (w2 / gcd_w) * (h1 / gcd_h) * (h2 / gcd_h) * sizeof(vx_float64);
        }
        /* printf("Requesting "VX_FMT_SIZE" bytes for resizer\n", size); */
#else
        size = 1;
#endif
        vxQueryKernel(node->kernel, VX_KERNEL_LOCAL_DATA_SIZE, &kernel_data_size, sizeof(kernel_data_size));
        if (kernel_data_size == 0)
        {
            node->attributes.localDataSize = size;
        }
        status = VX_SUCCESS;
    }
    return status;
}

vx_kernel_description_t scale_image_kernel =
{
    VX_KERNEL_SCALE_IMAGE,
    "org.khronos.openvx.scale_image",
    vxScaleImageKernel,
    scale_kernel_params, dimof(scale_kernel_params),
    NULL,
    vxScaleImageInputValidator,
    vxScaleImageOutputValidator,
    vxScaleImageInitializer,
    NULL,
};


/* half scale gaussian kernel */
static vx_status VX_CALLBACK vxHalfscaleGaussianKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    (void)parameters;

    if (num == dimof(scale_kernel_params))
    {
        vx_graph subgraph = ownGetChildGraphOfNode(node);
        status = vxProcessGraph(subgraph);
    }

    return status;
}

static vx_status VX_CALLBACK vxHalfscaleGaussianInputValidator(vx_node node, vx_uint32 index)
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
                if (stype == VX_TYPE_INT32)
                {
                    vx_int32 ksize = 0;
                    vxCopyScalar(scalar, &ksize, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
                    if ((ksize == 1) || (ksize == 3) || (ksize == 5))
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

static vx_status VX_CALLBACK vxHalfscaleGaussianOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if ((vxGetStatus((vx_reference)src_param) == VX_SUCCESS) &&
            (vxGetStatus((vx_reference)dst_param) == VX_SUCCESS))
        {
            vx_image src = 0;
            vx_image dst = 0;
            vxQueryParameter(src_param, VX_PARAMETER_REF, &src, sizeof(src));
            vxQueryParameter(dst_param, VX_PARAMETER_REF, &dst, sizeof(dst));
            if ((src) && (dst))
            {
                vx_uint32 w1 = 0, h1 = 0;
                vx_df_image f1 = VX_DF_IMAGE_VIRT;

                vxQueryImage(src, VX_IMAGE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(src, VX_IMAGE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(src, VX_IMAGE_FORMAT, &f1, sizeof(f1));

                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = f1;
                ptr->dim.image.width = (w1 + 1) / 2;
                ptr->dim.image.height = (h1 + 1) / 2;
                status = VX_SUCCESS;
            }
            if (src) vxReleaseImage(&src);
            if (dst) vxReleaseImage(&dst);
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

static const vx_uint32 gaussian5x5scale = 256;
static const vx_int16 gaussian5x5[5][5] =
{
    {1,  4,  6,  4, 1},
    {4, 16, 24, 16, 4},
    {6, 24, 36, 24, 6},
    {4, 16, 24, 16, 4},
    {1,  4,  6,  4, 1}
};

static vx_convolution vxCreateGaussian5x5Convolution(vx_context context)
{
    vx_convolution conv = vxCreateConvolution(context, 5, 5);
    vx_status status = vxCopyConvolutionCoefficients(conv, (vx_int16 *)gaussian5x5, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
    if (status != VX_SUCCESS)
    {
        vxReleaseConvolution(&conv);
        return NULL;
    }

    status = vxSetConvolutionAttribute(conv, VX_CONVOLUTION_SCALE, (void *)&gaussian5x5scale, sizeof(vx_uint32));
    if (status != VX_SUCCESS)
    {
        vxReleaseConvolution(&conv);
        return NULL;
    }
    return conv;
}

static vx_status VX_CALLBACK vxHalfscaleGaussianInitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;

    if (num == dimof(scale_kernel_params))
    {
        vx_context context = vxGetContext((vx_reference)node);

        vx_graph subgraph = node->child;
        if (subgraph)
        {
            /* deallocate subgraph resources */
            status = vxReleaseGraph(&subgraph);
            if (VX_SUCCESS != status)
                return status;

            status = ownSetChildGraphOfNode(node, 0);
            if (VX_SUCCESS != status)
                return status;
        }

        /* allocate subgraph resources */
        subgraph = vxCreateGraph(context);

        status = vxGetStatus((vx_reference)subgraph);
        if (status == VX_SUCCESS)
        {
            vx_uint32 i;
            vx_image input  = (vx_image)parameters[0];
            vx_image output = (vx_image)parameters[1];
            vx_int32 kernel_size = 3;
            vx_convolution convolution = 0;

            /* We have a child-graph; we want to make sure the parent
               graph is recognized as a valid scope for sake of virtual
               image parameters. */
            subgraph->parentGraph = node->graph;

            status |= vxCopyScalar((vx_scalar)parameters[2], &kernel_size, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
            if (kernel_size == 1)
            {
                    vx_node nodes[] =
                    {
                        vxScaleImageNode(subgraph, input, output, VX_INTERPOLATION_NEAREST_NEIGHBOR),
                    };

                    vx_border_t borders;
                    status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        status |= vxSetNodeAttribute(nodes[i], VX_NODE_BORDER, &borders, sizeof(borders));
                    }

                    status |= vxAddParameterToGraphByIndex(subgraph, nodes[0], 0); /* input image */
                    status |= vxAddParameterToGraphByIndex(subgraph, nodes[0], 1); /* output image */

                    status |= vxVerifyGraph(subgraph);

                    /* release our references, the graph will hold it's own */
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        status |= vxReleaseNode(&nodes[i]);
                    }

                    status |= ownSetChildGraphOfNode(node, subgraph);
            }
            else if (kernel_size == 3 || kernel_size == 5)
            {
                if (kernel_size == 5)
                {
                    convolution = vxCreateGaussian5x5Convolution(context);
                }
                if (kernel_size == 3 || convolution)
                {
                    vx_image virt = vxCreateVirtualImage(subgraph, 0, 0, VX_DF_IMAGE_U8);
                    vx_node nodes[] =
                    {
                            kernel_size == 3 ? vxGaussian3x3Node(subgraph, input, virt) : vxConvolveNode(subgraph, input, convolution, virt),
                            vxScaleImageNode(subgraph, virt, output, VX_INTERPOLATION_NEAREST_NEIGHBOR),
                    };

                    vx_border_t borders;
                    status |= vxQueryNode(node, VX_NODE_BORDER, &borders, sizeof(borders));
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        status |= vxSetNodeAttribute(nodes[i], VX_NODE_BORDER, &borders, sizeof(borders));
                    }

                    status |= vxAddParameterToGraphByIndex(subgraph, nodes[0], 0); /* input image */
                    status |= vxAddParameterToGraphByIndex(subgraph, nodes[1], 1); /* output image */
                    status |= vxAddParameterToGraphByIndex(subgraph, node, 2);     /* gradient size - refer to self to quiet sub-graph validator */

                    status |= vxVerifyGraph(subgraph);

                    /* release our references, the graph will hold it's own */
                    for (i = 0; i < dimof(nodes); i++)
                    {
                        status |= vxReleaseNode(&nodes[i]);
                    }

                    if (convolution)
                        status |= vxReleaseConvolution(&convolution);

                    status |= vxReleaseImage(&virt);

                    status |= ownSetChildGraphOfNode(node, subgraph);
                }
            }
        }
    }

    return status;
}

static vx_status VX_CALLBACK vxHalfscaleGaussianDeinitializer(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    (void)parameters;

    if (num == dimof(scale_kernel_params))
    {
        vx_graph subgraph = ownGetChildGraphOfNode(node);

        status = VX_SUCCESS;

        status |= vxReleaseGraph(&subgraph);

        /* set subgraph to "null" */
        status |= ownSetChildGraphOfNode(node, 0);
    }

    return status;
}

vx_kernel_description_t halfscale_gaussian_kernel =
{
    VX_KERNEL_HALFSCALE_GAUSSIAN,
    "org.khronos.openvx.halfscale_gaussian",
    vxHalfscaleGaussianKernel,
    scale_kernel_params, dimof(scale_kernel_params),
    NULL,
    vxHalfscaleGaussianInputValidator,
    vxHalfscaleGaussianOutputValidator,
    vxHalfscaleGaussianInitializer,
    vxHalfscaleGaussianDeinitializer,
};
