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

#ifdef OPENVX_CONFORMANCE_NEURAL_NETWORKS
#ifdef OPENVX_USE_NN

#include "vx_internal.h"

#include "VX/vx_khr_nn.h"


/*==============================================================================
vxConvolutionLayer
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxConvolutionLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights, vx_tensor biases,
        const vx_nn_convolution_params_t *convolution_params, vx_size size_of_convolution_params, vx_tensor outputs)
{
    (void)size_of_convolution_params;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar sc_pad_x = vxCreateScalar(context, VX_TYPE_SIZE, &convolution_params->padding_x);
    vx_scalar sc_pad_y = vxCreateScalar(context, VX_TYPE_SIZE, &convolution_params->padding_y);
    vx_scalar sc_overflow_policy = vxCreateScalar(context, VX_TYPE_ENUM, &convolution_params->overflow_policy);
    vx_scalar sc_rounding_policy = vxCreateScalar(context, VX_TYPE_ENUM, &convolution_params->rounding_policy);
    vx_scalar sc_down_scale_size_rounding = vxCreateScalar(context, VX_TYPE_ENUM, &convolution_params->down_scale_size_rounding);
    vx_scalar sc_dilation_x = vxCreateScalar(context, VX_TYPE_SIZE, &convolution_params->dilation_x);
    vx_scalar sc_dilation_y = vxCreateScalar(context, VX_TYPE_SIZE, &convolution_params->dilation_y);
    vx_reference params[] = {
        (vx_reference)inputs,
        (vx_reference)weights,
        (vx_reference)biases,
        (vx_reference)sc_pad_x,
        (vx_reference)sc_pad_y,
        (vx_reference)sc_overflow_policy,
        (vx_reference)sc_rounding_policy,
        (vx_reference)sc_down_scale_size_rounding,
        (vx_reference)sc_dilation_x,
        (vx_reference)sc_dilation_y,
        (vx_reference)outputs,
    };
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_CONVOLUTION_LAYER, params, dimof(params));

    vxReleaseScalar(&sc_pad_x);
    vxReleaseScalar(&sc_pad_y);
    vxReleaseScalar(&sc_overflow_policy);
    vxReleaseScalar(&sc_rounding_policy);
    vxReleaseScalar(&sc_down_scale_size_rounding);
    vxReleaseScalar(&sc_dilation_x);
    vxReleaseScalar(&sc_dilation_y);
    return node;
}


/*==============================================================================
vxPoolingLayer
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxPoolingLayer(vx_graph graph, vx_tensor inputs, vx_enum pooling_type,
		vx_size pooling_size_x,
		vx_size pooling_size_y,
		vx_size pooling_padding_x,
		vx_size pooling_padding_y,
		vx_enum rounding,
		vx_tensor outputs)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar pooling_type_scalar = vxCreateScalar(context, VX_TYPE_ENUM, &pooling_type);
    vx_scalar pool_size_x_scalar = vxCreateScalar(context, VX_TYPE_SIZE, &pooling_size_x);
    vx_scalar pool_size_y_scalar = vxCreateScalar(context, VX_TYPE_SIZE, &pooling_size_y);
    vx_scalar pool_pad_x_scalar = vxCreateScalar(context, VX_TYPE_SIZE, &pooling_padding_x);
    vx_scalar pool_pad_y_scalar = vxCreateScalar(context, VX_TYPE_SIZE, &pooling_padding_y);
    vx_scalar rounding_scalar = vxCreateScalar(context, VX_TYPE_ENUM, &rounding);
    vx_reference params[] = {
        (vx_reference)inputs,
        (vx_reference)pooling_type_scalar,
        (vx_reference)pool_size_x_scalar,
        (vx_reference)pool_size_y_scalar,
        (vx_reference)pool_pad_x_scalar,
        (vx_reference)pool_pad_y_scalar,
        (vx_reference)rounding_scalar,
        (vx_reference)outputs,
    };
    
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_POOLING_LAYER, params, dimof(params));
    vxReleaseScalar(&pooling_type_scalar);
    vxReleaseScalar(&pool_size_x_scalar);
    vxReleaseScalar(&pool_size_y_scalar);
    vxReleaseScalar(&pool_pad_x_scalar);
    vxReleaseScalar(&pool_pad_y_scalar);
    vxReleaseScalar(&rounding_scalar);
    return node;
}


/*==============================================================================
vxFullyConnectedLayer
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxFullyConnectedLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights, vx_tensor biases,
		vx_enum overflow_policy,
		vx_enum rounding_policy,
		vx_tensor outputs)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar sc_overflow_policy = vxCreateScalar(context, VX_TYPE_ENUM, &overflow_policy);
    vx_scalar sc_rounding_policy = vxCreateScalar(context, VX_TYPE_ENUM, &rounding_policy);
    vx_reference params[] = {
        (vx_reference)inputs,
        (vx_reference)weights,
        (vx_reference)biases,
        (vx_reference)sc_overflow_policy,
        (vx_reference)sc_rounding_policy,
        (vx_reference)outputs,
    };
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_FULLY_CONNECTED_LAYER, params, dimof(params));
    vxReleaseScalar(&sc_overflow_policy);
    vxReleaseScalar(&sc_rounding_policy);
    return node;
}




/*==============================================================================
vxSoftmaxLayer
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxSoftmaxLayer(vx_graph graph, vx_tensor inputs, vx_tensor outputs)
{
    vx_reference params[] = {
        (vx_reference)inputs,
        (vx_reference)outputs,
    };
    return vxCreateNodeByStructure(graph, VX_KERNEL_SOFTMAX_LAYER, params, dimof(params));
}


/*==============================================================================
vxLocalResponseNormalizationLayer
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxLocalResponseNormalizationLayer(vx_graph graph, vx_tensor inputs, vx_enum type,
		vx_size normalization_size,
		vx_float32 alpha,
		vx_float32 beta,
		vx_float32 bias,
		vx_tensor outputs)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar type_scalar = vxCreateScalar(context, VX_TYPE_ENUM, &type);
    vx_scalar norm_size_scalar = vxCreateScalar(context, VX_TYPE_SIZE, &normalization_size);
    vx_scalar alpha_scalar = vxCreateScalar(context, VX_TYPE_FLOAT32, &alpha);
    vx_scalar beta_scalar = vxCreateScalar(context, VX_TYPE_FLOAT32, &beta);
    vx_reference params[] = {
        (vx_reference)inputs,
        (vx_reference)type_scalar,
        (vx_reference)norm_size_scalar,
        (vx_reference)alpha_scalar,
        (vx_reference)beta_scalar,
        (vx_reference)outputs,
    };
    
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_NORMALIZATION_LAYER, params, dimof(params));
    vxReleaseScalar(&type_scalar);
    vxReleaseScalar(&norm_size_scalar);
    vxReleaseScalar(&alpha_scalar);
    vxReleaseScalar(&beta_scalar);
    return node;
}


/*==============================================================================
vxActivationLayer
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxActivationLayer(vx_graph graph, vx_tensor inputs, vx_enum function, vx_float32 a,vx_float32 b, vx_tensor outputs) {
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar func_scalar = vxCreateScalar(context, VX_TYPE_ENUM, &function);
    vx_scalar a_scalar = vxCreateScalar(context, VX_TYPE_FLOAT32, &a);
    vx_scalar b_scalar = vxCreateScalar(context, VX_TYPE_FLOAT32, &b);
    vx_reference params[] = {
        (vx_reference)inputs,
        (vx_reference)func_scalar,
        (vx_reference)a_scalar,
        (vx_reference)b_scalar,
        (vx_reference)outputs,
    };
    
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_ACTIVATION_LAYER, params, dimof(params));
    vxReleaseScalar(&func_scalar);
    vxReleaseScalar(&a_scalar);
    vxReleaseScalar(&b_scalar);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxROIPoolingLayer(
        vx_graph graph,
        vx_tensor input_data,
        vx_tensor input_rois,
        const vx_nn_roi_pool_params_t * roi_pool_params,
        vx_size size_of_roi_params,
        vx_tensor output_arr)
{
    vx_node node = NULL;
    
    if (size_of_roi_params == sizeof(vx_enum))
    {
        vx_context context = vxGetContext((vx_reference)graph);

        vx_enum pool_type = roi_pool_params->pool_type;
        vx_scalar pool_type_sc = vxCreateScalar(context, VX_TYPE_ENUM, &pool_type);

        vx_reference params[] = {
            (vx_reference)input_data,
            (vx_reference)input_rois,
            (vx_reference)pool_type_sc,
            (vx_reference)output_arr,
        };
        node = vxCreateNodeByStructure(graph, VX_KERNEL_ROI_POOLING_LAYER, params, dimof(params));

        vxReleaseScalar(&pool_type_sc);
    }

    return node;
}

/*==============================================================================
vxDeconvolutionLayer
=============================================================================*/
VX_API_ENTRY vx_node VX_API_CALL vxDeconvolutionLayer(vx_graph graph, vx_tensor inputs, vx_tensor weights, vx_tensor  biases,
        const vx_nn_deconvolution_params_t *deconvolution_params, vx_size size_of_deconv_params, vx_tensor outputs)

{
    (void)size_of_deconv_params;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar sc_pad_x = vxCreateScalar(context, VX_TYPE_SIZE, &deconvolution_params->padding_x);
    vx_scalar sc_pad_y = vxCreateScalar(context, VX_TYPE_SIZE, &deconvolution_params->padding_y);
    vx_scalar sc_overflow_policy = vxCreateScalar(context, VX_TYPE_ENUM, &deconvolution_params->overflow_policy);
    vx_scalar sc_rounding_policy = vxCreateScalar(context, VX_TYPE_ENUM, &deconvolution_params->rounding_policy);
    vx_scalar sc_a_x = vxCreateScalar(context, VX_TYPE_SIZE, &deconvolution_params->a_x);
    vx_scalar sc_a_y = vxCreateScalar(context, VX_TYPE_SIZE, &deconvolution_params->a_y);
    vx_reference params[] = {
        (vx_reference)inputs,
        (vx_reference)weights,
        (vx_reference)biases,
        (vx_reference)sc_pad_x,
        (vx_reference)sc_pad_y,
        (vx_reference)sc_overflow_policy,
        (vx_reference)sc_rounding_policy,
        (vx_reference)sc_a_x,
        (vx_reference)sc_a_y,
        (vx_reference)outputs,
    };
    vx_node node = vxCreateNodeByStructure(graph, VX_KERNEL_DECONVOLUTION_LAYER, params, dimof(params));
    vxReleaseScalar(&sc_pad_x);
    vxReleaseScalar(&sc_pad_y);
    vxReleaseScalar(&sc_overflow_policy);
    vxReleaseScalar(&sc_rounding_policy);
    vxReleaseScalar(&sc_a_x);
    vxReleaseScalar(&sc_a_y);
    return node;
}

#endif
#endif//OPENVX_CONFORMANCE_NEURAL_NETWORKS
