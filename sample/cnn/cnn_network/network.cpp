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
#include "network.h"

#include <VX/vx.h>
#include <VX/vxu.h> //TODO(ruv): this is depracated, must the sample here use it?
#include <VX/vx_khr_nn.h>
#include "half/sip_ml_fp16.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iomanip>      // std::setfill, std::setw
#include <sstream>
#include <string>
#include <vector>


#define NUM_OF_LAYERS_OVERFEAT           18
#define NUM_OF_LAYERS_ALEXNET            21
#define NUM_OF_UNMIRRORED_PATCHES_ALEXNET 5
#define NUM_OF_LAYERS_GOOGLENET2         13
#define NUM_OF_LAYERS_BDS                5
#define NUM_OF_LAYERS_BDS_RNN            350
#define BDS_BATCH_SIZE                   2
#define BDS_NUM_OFMS                    3//2304

#define _CNN_DUMP_


typedef struct _pooling_params
{
    vx_size pooling_size_x;
    vx_size pooling_size_y;
    vx_size pooling_padding_x;
    vx_size pooling_padding_y;
    vx_enum rounding;
} pooling_params;

typedef struct _convolution_params
{
    vx_size padding_x;
    vx_size padding_y;
    vx_enum overflow_policy;
    vx_enum rounding_policy;
    vx_enum down_scale_size_rounding;

} convolution_params;

typedef struct _activation_params
{
    vx_enum function;
    vx_float32 a;
    vx_float32 b;
} activation_params;
typedef struct _normalization_params
{
    vx_enum type;
    vx_size normalization_size;
    vx_float32 alpha;
    vx_float32 beta;
} normalization_params;

#ifdef _WIN32
#ifdef _WIN64
typedef __int64 LONG_PTR;
#else
typedef long LONG_PTR;
#endif
typedef LONG_PTR SSIZE_T;
typedef SSIZE_T ssize_t;
#endif

size_t CopyToFromTensor(vx_context context, vx_tensor tensor, int16_t *buffer,
        ssize_t bufSize, vx_accessor_e accesor);
// Convert from Q1.7.8 fixed point decimal into regular decimal 
float short2float(int16_t val) {
    int16_t sign = (val & 0x8000) >> 15;

    if (sign == 1) {
        val ^= 0xffff;
        val++;
    }
    int16_t mantisa = (val & 0x7F00) >> 8;
    int16_t fraction = (val & 0x00FF);

    int16_t checker = 0x0001;
    float counter = 0;
    for (int i = 8; i > 0; i--) {
        if ((fraction & checker) != 0) {
            counter += 1.0f / (1 << i);
        }
        checker <<= 1;
    }

    counter += mantisa;

    if (sign == 1) {
        counter = -counter;
    }

    return counter;
}


void dumpToFile(vx_tensor md_data, int id, std::string name = "")
{
#ifdef _CNN_DUMP_
    vx_status status = VX_SUCCESS;
    if (md_data != NULL)
    {
        char filename[256];
        sprintf (filename, "%s%d.md", name.c_str(), id);
        FILE* f = fopen(filename, "wb");
        vx_int16* buffer;
        vx_size dims_num;
        status |= vxQueryTensor(md_data, VX_TENSOR_NUMBER_OF_DIMS, &dims_num, sizeof(dims_num));
        std::vector<vx_size> dims(dims_num); dims.resize(dims_num); //vx_size dims[dims_num];

        status |= vxQueryTensor(md_data, VX_TENSOR_DIMS, &dims[0], dims.size()*sizeof(vx_size));
        vx_size size = 1;
        for (vx_size i = 0; i < dims_num; i++)
        {
            size = size*dims[i];
        }
        buffer = (vx_int16*)malloc(size*sizeof(vx_int16));
        vx_context context = vxGetContext((vx_reference)md_data);
        CopyToFromTensor(context, md_data, buffer, size*sizeof(vx_int16), VX_READ_ONLY);
        fwrite((vx_int16 *)buffer, sizeof(int16_t), size, f);

        fclose(f);
        free (buffer);
    }


#endif // _CNN_DUMP_
}



size_t CopyToFromTensor(vx_context context, vx_tensor tensor, int16_t *buffer,
        ssize_t bufSize, vx_accessor_e accesor) {
    vx_status status = VX_SUCCESS;
    vx_size numDims;
    status = vxQueryTensor(tensor, VX_TENSOR_NUMBER_OF_DIMS, &numDims,
            sizeof(numDims));
    vx_type_e data_type;
    status |= vxQueryTensor(tensor, VX_TENSOR_DATA_TYPE, &data_type, sizeof(data_type));
    vx_uint8 fixed_point_pos = 0;
    status |= vxQueryTensor(tensor, VX_TENSOR_FIXED_POINT_POSITION, &fixed_point_pos, sizeof(fixed_point_pos));

    //TODO: use validFormat instead.
#ifdef EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT
    if ((data_type == VX_TYPE_INT16 && fixed_point_pos == Q78_FIXED_POINT_POSITION) || data_type == VX_TYPE_FLOAT16)
#else
    if ((data_type == VX_TYPE_INT16) && (fixed_point_pos == Q78_FIXED_POINT_POSITION))
#endif
    {
        vx_size elementSize = 2;
        std::vector<vx_size> dims; dims.resize(numDims);        // vx_size dims[numDims];
        std::vector<vx_size> start; start.resize(numDims);      // vx_size start[numDims];
        std::vector<vx_size> strides; strides.resize(numDims);  // vx_size strides[numDims];
        status |= vxQueryTensor(tensor, VX_TENSOR_DIMS, &dims[0], dims.size()*sizeof(vx_size));
        vx_size totalSize = 0;
        for (vx_size i = 0; i < numDims; i++) {
            start[i] = 0;
            if (i == 0)
                strides[i] = elementSize;
            else
                strides[i] = strides[i - 1] * dims[i - 1];
        }
        if ((bufSize == -1)
                || (bufSize == strides[numDims - 1] * dims[numDims - 1])) {
            status = vxCopyTensorPatch(tensor, numDims, &start[0], &dims[0], &strides[0], buffer, accesor,
                    VX_MEMORY_TYPE_HOST);
            bufSize = strides[numDims - 1] * dims[numDims - 1]/elementSize;
        } else {
            status = VX_FAILURE;
        }
    }

    return (status == VX_SUCCESS) ? bufSize : 0;
}

void preprocessImageNetQ78(vx_context context, uint8_t *pinput, vx_uint32 size,
        vx_object_array img_array) {
    bool immediate = false;
    vx_status status = VX_SUCCESS;
    vx_pixel_value_t avg;
    avg.S16 = (int16_t) (256.0 * -118.380948);
    vx_pixel_value_t sd;
    sd.S16 = (int16_t) (256.0 * 256.0 / 61.896913);

    vx_uint32 width = size;
    vx_uint32 height = size;

    vx_image image = vxCreateImage(context, width, height, VX_DF_IMAGE_RGB);
    {
        vx_rectangle_t rect = { 0, 0, width, height };
        vx_imagepatch_addressing_t addr;

        status |= vxCopyImagePatch(image, &rect, 0, &addr, pinput, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); 
    }

    vx_float32 scale = 1.0 / (256 * 256);
    vx_int32 shift = 7;

    printf("Preparing input. Immediate=%d\n", immediate);

    vx_image img_r, img_g, img_b;
    img_r = (vx_image) vxGetObjectArrayItem(img_array, 0);
    img_g = (vx_image) vxGetObjectArrayItem(img_array, 1);
    img_b = (vx_image) vxGetObjectArrayItem(img_array, 2);

    if (immediate) {
        vx_image img1 = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
        vx_image img2 = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
        vx_image img3 = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
        vx_image img1s = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
        vx_image img2s = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
        vx_image img3s = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
        vx_image img1sa = vxCreateImage(context, width, height,
                VX_DF_IMAGE_S16);
        vx_image img2sa = vxCreateImage(context, width, height,
                VX_DF_IMAGE_S16);
        vx_image img3sa = vxCreateImage(context, width, height,
                VX_DF_IMAGE_S16);
        vx_image img_avg = vxCreateUniformImage(context, width, height,
                VX_DF_IMAGE_S16, &avg);
        vx_image img_sd = vxCreateUniformImage(context, width, height,
                VX_DF_IMAGE_S16, &sd);

        status |= vxuChannelExtract(context, image, VX_CHANNEL_R, img1);
        status |= vxuChannelExtract(context, image, VX_CHANNEL_G, img2);
        status |= vxuChannelExtract(context, image, VX_CHANNEL_B, img3);

        status |= vxuConvertDepth(context, img1, img1s,
                VX_CONVERT_POLICY_SATURATE, shift);
        status |= vxuConvertDepth(context, img2, img2s,
                VX_CONVERT_POLICY_SATURATE, shift);
        status |= vxuConvertDepth(context, img3, img3s,
                VX_CONVERT_POLICY_SATURATE, shift);

        status |= vxuAdd(context, img1s, img_avg, VX_CONVERT_POLICY_SATURATE,
                img1sa);
        status |= vxuAdd(context, img2s, img_avg, VX_CONVERT_POLICY_SATURATE,
                img2sa);
        status |= vxuAdd(context, img3s, img_avg, VX_CONVERT_POLICY_SATURATE,
                img3sa);

        status |= vxuMultiply(context, img1sa, img_sd, scale,
                VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_r);
        status |= vxuMultiply(context, img2sa, img_sd, scale,
                VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_g);
        status |= vxuMultiply(context, img3sa, img_sd, scale,
                VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_b);
    } else {
        vx_graph graph = vxCreateGraph(context);
        if (graph) {

            vx_image img1 = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_U8);
            vx_image img2 = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_U8);
            vx_image img3 = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_U8);
            vx_image img1s = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img2s = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img3s = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img1sa = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img2sa = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img3sa = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img_avg = vxCreateUniformImage(context, width, height,
                    VX_DF_IMAGE_S16, &avg);
            vx_image img_sd = vxCreateUniformImage(context, width, height,
                    VX_DF_IMAGE_S16, &sd);

            vx_scalar scale_sc = vxCreateScalar(context, VX_TYPE_FLOAT32,
                    &scale);
            vx_scalar shift_sc = vxCreateScalar(context, VX_TYPE_INT32, &shift);

            vx_node cnn_nodes[] = { vxChannelExtractNode(graph, image, VX_CHANNEL_R, img1),
                    vxChannelExtractNode(graph, image, VX_CHANNEL_G, img2),
                    vxChannelExtractNode(graph, image, VX_CHANNEL_B, img3),
                    vxConvertDepthNode(graph, img1, img1s, VX_CONVERT_POLICY_SATURATE, shift_sc),
                    vxConvertDepthNode(graph, img2, img2s, VX_CONVERT_POLICY_SATURATE, shift_sc),
                    vxConvertDepthNode(graph, img3, img3s, VX_CONVERT_POLICY_SATURATE, shift_sc),
                    vxAddNode(graph, img1s, img_avg, VX_CONVERT_POLICY_SATURATE, img1sa),
                    vxAddNode(graph, img2s, img_avg, VX_CONVERT_POLICY_SATURATE, img2sa),
                    vxAddNode(graph, img3s, img_avg, VX_CONVERT_POLICY_SATURATE, img3sa),
                    vxMultiplyNode(graph, img1sa, img_sd, scale_sc, VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_r),
                    vxMultiplyNode(graph, img2sa, img_sd, scale_sc,VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_g),
                    vxMultiplyNode(graph, img3sa, img_sd, scale_sc, VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_b), };

            status |= vxVerifyGraph(graph);
            if (status == VX_SUCCESS) {
                status = vxProcessGraph(graph);
            }

            for (int n = 0; n < sizeof(cnn_nodes) / sizeof(cnn_nodes[0]); n++) {
                vxReleaseNode(&cnn_nodes[n]);
            }

            vxReleaseGraph(&graph);
        }
    }

    printf("Input ready\n");
}

void preprocessOrigImageAlexnet(vx_context context, uint8_t *pinput,
        uint8_t *pavgr, uint8_t *pavgg, uint8_t *pavgb,
        vx_object_array img_array) {
    bool immediate = false;
    vx_status status = VX_SUCCESS;
    vx_uint32 width = 256, height = 256, avg_dim = 256;

    vx_image image = vxCreateImage(context, width, height, VX_DF_IMAGE_RGB);
    {
        vx_rectangle_t rect = { 0, 0, width, height };
        vx_imagepatch_addressing_t addr;

        status |= vxCopyImagePatch(image, &rect, 0, &addr, pinput, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); 
    }

    vx_image img_avgr = vxCreateImage(context, avg_dim, avg_dim, VX_DF_IMAGE_S16);
    vx_image img_avgg = vxCreateImage(context, avg_dim, avg_dim, VX_DF_IMAGE_S16);
    vx_image img_avgb = vxCreateImage(context, avg_dim, avg_dim, VX_DF_IMAGE_S16);
    {
        vx_rectangle_t rect = { 0, 0, avg_dim, avg_dim };
        vx_imagepatch_addressing_t addr;

        status |= vxCopyImagePatch(img_avgr, &rect, 0, &addr, pavgr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); 
        status |= vxCopyImagePatch(img_avgg, &rect, 0, &addr, pavgg, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); 
        status |= vxCopyImagePatch(img_avgb, &rect, 0, &addr, pavgb, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST); 
    }

    // shifting 7 bits instead of 8 so we don't lose input's msb
    // since we're doing it before the subtraction of the average
    vx_int32 shift = 7;
    // shifting by one more bit after substracting the average
    vx_pixel_value_t scale;
    scale.S16 = 2;

    printf("Preparing input. Immediate=%d\n", immediate);

    vx_image img_r, img_g, img_b;
    img_r = (vx_image) vxGetObjectArrayItem(img_array, 0);
    img_g = (vx_image) vxGetObjectArrayItem(img_array, 1);
    img_b = (vx_image) vxGetObjectArrayItem(img_array, 2);
    if (immediate) {
        vx_image img1 = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
        vx_image img2 = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
        vx_image img3 = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
        vx_image img1s = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
        vx_image img2s = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
        vx_image img3s = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
        vx_image img1sa = vxCreateImage(context, width, height,
                VX_DF_IMAGE_S16);
        vx_image img2sa = vxCreateImage(context, width, height,
                VX_DF_IMAGE_S16);
        vx_image img3sa = vxCreateImage(context, width, height,
                VX_DF_IMAGE_S16);
        vx_image img_scale = vxCreateUniformImage(context, width, height,
                VX_DF_IMAGE_S16, &scale);

        status |= vxuChannelExtract(context, image, VX_CHANNEL_R, img1);
        status |= vxuChannelExtract(context, image, VX_CHANNEL_G, img2);
        status |= vxuChannelExtract(context, image, VX_CHANNEL_B, img3);

        status |= vxuConvertDepth(context, img1, img1s,
                VX_CONVERT_POLICY_SATURATE, shift);
        status |= vxuConvertDepth(context, img2, img2s,
                VX_CONVERT_POLICY_SATURATE, shift);
        status |= vxuConvertDepth(context, img3, img3s,
                VX_CONVERT_POLICY_SATURATE, shift);

        status |= vxuSubtract(context, img1s, img_avgr,
                VX_CONVERT_POLICY_SATURATE, img1sa);
        status |= vxuSubtract(context, img2s, img_avgg,
                VX_CONVERT_POLICY_SATURATE, img2sa);
        status |= vxuSubtract(context, img3s, img_avgb,
                VX_CONVERT_POLICY_SATURATE, img3sa);

        status |= vxuMultiply(context, img1sa, img_scale, 1.0,
                VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_r);
        status |= vxuMultiply(context, img2sa, img_scale, 1.0,
                VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_g);
        status |= vxuMultiply(context, img3sa, img_scale, 1.0,
                VX_CONVERT_POLICY_SATURATE, VX_ROUND_POLICY_TO_ZERO, img_b);
    } else {
        vx_graph graph = vxCreateGraph(context);
        if (graph) {

            vx_image img1 = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_U8);
            vx_image img2 = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_U8);
            vx_image img3 = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_U8);
            vx_image img1s = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img2s = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img3s = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img1sa = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img2sa = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);
            vx_image img3sa = vxCreateVirtualImage(graph, width, height,
                    VX_DF_IMAGE_S16);

            vx_scalar shift_sc = vxCreateScalar(context, VX_TYPE_INT32, &shift);

            vx_node cnn_nodes[] = { vxChannelExtractNode(graph, image,
                    VX_CHANNEL_R, img1), vxChannelExtractNode(graph, image,
                            VX_CHANNEL_G, img2), vxChannelExtractNode(graph, image,
                                    VX_CHANNEL_B, img3), vxConvertDepthNode(graph, img1, img1s,
                                            VX_CONVERT_POLICY_SATURATE, shift_sc), vxConvertDepthNode(
                                                    graph, img2, img2s, VX_CONVERT_POLICY_SATURATE, shift_sc),
                                                    vxConvertDepthNode(graph, img3, img3s,
                                                            VX_CONVERT_POLICY_SATURATE, shift_sc),
                                                            vxSubtractNode(graph, img1s, img_avgr,
                                                                    VX_CONVERT_POLICY_SATURATE, img_r), vxSubtractNode(
                                                                            graph, img2s, img_avgg, VX_CONVERT_POLICY_SATURATE,
                                                                            img_g), vxSubtractNode(graph, img3s, img_avgb,
                                                                                    VX_CONVERT_POLICY_SATURATE, img_b), };

            status |= vxVerifyGraph(graph);
            if (status == VX_SUCCESS) {
                status = vxProcessGraph(graph);
            }

            for (int n = 0; n < sizeof(cnn_nodes) / sizeof(cnn_nodes[0]); n++) {
                vxReleaseNode(&cnn_nodes[n]);
            }

            vxReleaseGraph(&graph);
        }
    }

    printf("Input ready\n");
}


int Overfeat(int16_t** ppdata, vx_enum format, vx_int8 fp_pos, bool raw,
        int16_t *output) {
    vx_bool dump = vx_false_e;

    // Definitions
    vx_size wt_dims[NUM_OF_LAYERS_OVERFEAT][6] = { { 11, 11, 3, 96, 0, 0 }, {0,0,0,0,0,0}, {0,0,0,0,0,0},
            {5, 5, 96, 256, 0, 0 }, {0,0,0,0,0,0}, {0,0,0,0,0,0},
            { 3, 3, 256, 512, 0, 0 }, {0,0,0,0,0,0},
            { 3, 3, 512, 1024, 0, 0 }, {0,0,0,0,0,0},
            { 3, 3, 1024, 1024, 0, 0 }, {0,0,0,0,0,0}, {0,0,0,0,0,0},
            { 6, 6, 1024, 3072, 0, 0 }, {0,0,0,0,0,0},
            {3072, 4096, 0, 0, 0, 0 }, {0,0,0,0,0,0},
            { 4096, 1000, 0, 0, 0, 0 }, };

    pooling_params pool_params = {2,2,0,0,VX_NN_DS_SIZE_ROUNDING_FLOOR};
    activation_params relu_params = {VX_NN_ACTIVATION_RELU, 0, 0};
    vx_nn_convolution_params_t conv_params[3] = {{0,0,VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, VX_NN_DS_SIZE_ROUNDING_FLOOR, 0, 0},
            {0,0,VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, VX_NN_DS_SIZE_ROUNDING_FLOOR, 0, 0},
            {1,1,VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, VX_NN_DS_SIZE_ROUNDING_FLOOR, 0, 0}};
    vx_size out_dims[NUM_OF_LAYERS_OVERFEAT][6] = { {56, 56, 96, 0, 0, 0}, {56, 56, 96, 0, 0, 0}, { 28, 28, 96, 0, 0, 0 },
            {24, 24, 256, 0, 0, 0 }, {24, 24, 256, 0, 0, 0 }, {12, 12, 256, 0, 0, 0 },
            { 12, 12, 512, 0, 0, 0 }, { 12, 12, 512, 0, 0, 0 },
            { 12, 12, 1024, 0, 0, 0 }, { 12, 12, 1024, 0, 0, 0 },
            { 12, 12, 1024, 0, 0, 0 }, { 12, 12, 1024, 0, 0, 0 }, { 6, 6, 1024, 0, 0, 0 },
            { 1, 1, 3072, 0, 0, 0 }, { 1, 1, 3072, 0, 0, 0 },
            { 4096, 0, 0, 0, 0, 0 }, { 4096, 0, 0, 0, 0, 0 },
            { 1000, 0, 0, 0, 0, 0 }, };

    // Get input
    uint8_t *pInput = *(uint8_t**) ppdata;

    // Get weights
    int16_t *pInt16Params = (int16_t *) *(ppdata + 1);

    size_t offset = 0;

    // Building overfeat network
    // Edges
    vx_status status = VX_SUCCESS;
    vx_context context = vxCreateContext();

    vx_size dim_max = 0;
    vxQueryContext(context, VX_CONTEXT_MAX_TENSOR_DIMS, &dim_max,
            sizeof(dim_max));
    vx_graph graph;
    graph = vxCreateGraph(context);

    void *ptr = NULL;

    vx_tensor wt_data[NUM_OF_LAYERS_OVERFEAT] = { 0 };
    vx_tensor bs_data[NUM_OF_LAYERS_OVERFEAT] = { 0 };
    vx_tensor out_data[NUM_OF_LAYERS_OVERFEAT] = { 0 };

    printf("Building MD Datas\n");
    vx_size sizes[3] = { 231, 231, 3 };
    vx_tensor in = vxCreateTensor(context, 3, sizes, format, fp_pos);
    void *base = NULL;
    if (raw) {
        if (format == VX_TYPE_INT16) {
            // if patch was not preprocessed yet - preprocess it
            vx_rectangle_t rect = { 0, 0, 231, 231 };
            vx_object_array inImgs = vxCreateImageObjectArrayFromTensor(in,
                    &rect, 3, 1, VX_DF_IMAGE_S16);
            preprocessImageNetQ78(context, pInput, 231, inImgs);
            vxReleaseObjectArray(&inImgs);
        }
#ifdef EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT
        else if (format == VX_TYPE_FLOAT16) {
            preprocessImageNetF16(context, pInput, in);
        }
#endif
    } else {
        status |= (CopyToFromTensor(context, in, (int16_t*) pInput, -1,
                VX_WRITE_ONLY) <= 0);
        // We received a preprocessed patch - use as is
    }

    int out_dim = 0;
    int wo_wt = 0;
    for (int i = 0; i < NUM_OF_LAYERS_OVERFEAT; i++) {
        int wt_dim = 0;
        size_t wt_size = 2; //TODO: shouldn't be hardcoded 2!!!
        out_dim = 0;
        for (int j = 0; j < dim_max; j++) {
            if (wt_dims[i][j] > 0) {
                wt_dim++;
                wt_size *= wt_dims[i][j];
            }

            if (out_dims[i][j] > 0)
                out_dim++;
        }

        if (wt_dim > 0) {
            wt_data[i-wo_wt] = vxCreateTensor(context, wt_dim, wt_dims[i], format,
                    fp_pos);
            offset += CopyToFromTensor(context, wt_data[i-wo_wt],
                    pInt16Params + offset, wt_size, VX_WRITE_ONLY);
            bs_data[i-wo_wt] = vxCreateTensor(context, 1, &wt_dims[i][wt_dim - 1],
                    format, fp_pos);
            offset += CopyToFromTensor(context, bs_data[i-wo_wt],
                    pInt16Params + offset, 2*wt_dims[i][wt_dim - 1],
                    VX_WRITE_ONLY);
        } else {
            wo_wt ++;
        }

        if (out_dim > 0) {
            out_data[i] = vxCreateTensor(context, out_dim, out_dims[i],
                    format, fp_pos);
            //			out_data[i] = vxCreateVirtualTensor(graph, out_dim, out_dims[i],
            //					format, fp_pos);
        }
    }

    vx_tensor out = vxCreateTensor(context, out_dim,
            out_dims[NUM_OF_LAYERS_OVERFEAT - 1], format, fp_pos);



    printf("MD Datas ready\n");
    printf("Building and processing graph.\n");

    //Nodes
    if (graph) {
        vx_node cnn_nodes[] = {
                // Layer (1) - Conv + Relu + MAX
                // Convolution - 3X231X231 --> 96X56X56
                vxConvolutionLayer(graph, in, wt_data[0], bs_data[0], &conv_params[0], sizeof(vx_nn_convolution_params_t),out_data[0]),
                vxActivationLayer(graph, out_data[0], relu_params.function, relu_params.a, relu_params.b, out_data[1]),
                vxPoolingLayer(graph, out_data[1], VX_NN_POOLING_MAX, pool_params.pooling_size_x, pool_params.pooling_size_y,
                        pool_params.pooling_padding_x, pool_params.pooling_padding_y, pool_params.rounding,out_data[2]),
                // Layer (2) - Conv + Relu + MAX
                // Convolution - 96*28*28 --> 256*24*24
                vxConvolutionLayer(graph, out_data[2], wt_data[1], bs_data[1], &conv_params[1], sizeof(vx_nn_convolution_params_t),out_data[3]),
                vxActivationLayer(graph, out_data[3], relu_params.function, relu_params.a, relu_params.b, out_data[4]),
                vxPoolingLayer(graph, out_data[4], VX_NN_POOLING_MAX, pool_params.pooling_size_x, pool_params.pooling_size_y,
                        pool_params.pooling_padding_x, pool_params.pooling_padding_y, pool_params.rounding,out_data[5]),
                // Layer (3) - Conv + Relu
                // Convolution - 256X14X14 -> 512X12X12
                vxConvolutionLayer(graph, out_data[5], wt_data[2], bs_data[2], &conv_params[2], sizeof(vx_nn_convolution_params_t), out_data[6]),
                vxActivationLayer(graph, out_data[6], relu_params.function, relu_params.a, relu_params.b, out_data[7]),
                // Layer (4) - Conv + Relu
                // Convolution - 512X14X14 -> 1024*12*12
                vxConvolutionLayer(graph, out_data[7], wt_data[3], bs_data[3], &conv_params[2], sizeof(vx_nn_convolution_params_t),out_data[8]),
                vxActivationLayer(graph, out_data[8], relu_params.function, relu_params.a, relu_params.b, out_data[9]),
                // Layer (5) - Conv + Relu + MAX
                // Convolution - 1024*14*14 --> 1024*12*12
                vxConvolutionLayer(graph, out_data[9], wt_data[4], bs_data[4], &conv_params[2], sizeof(vx_nn_convolution_params_t),out_data[10]),
                vxActivationLayer(graph, out_data[10], relu_params.function, relu_params.a, relu_params.b, out_data[11]),
                vxPoolingLayer(graph, out_data[11], VX_NN_POOLING_MAX, pool_params.pooling_size_x, pool_params.pooling_size_y,
                        pool_params.pooling_padding_x, pool_params.pooling_padding_y, pool_params.rounding,out_data[12]),
                // Layer (6) - Conv + Relu
                // Convolution - 1024*6*6 --> 3072*1*1
                vxConvolutionLayer(graph, out_data[12], wt_data[5], bs_data[5], &conv_params[1], sizeof(vx_nn_convolution_params_t),out_data[13]),
                vxActivationLayer(graph, out_data[13], relu_params.function, relu_params.a, relu_params.b, out_data[14]),
                // Layer (7) - FullyConnected + Relu
                //Fully connected- 3072 -> 4096
                vxFullyConnectedLayer(graph, out_data[14], wt_data[6], bs_data[6], conv_params[0].overflow_policy,
                          conv_params[0].rounding_policy, out_data[15]),
                vxActivationLayer(graph, out_data[15], relu_params.function, relu_params.a, relu_params.b, out_data[16]),

                // Layer (8) - FullyConnected 4096 --> 1000
                vxFullyConnectedLayer(graph, out_data[16], wt_data[7],
                          bs_data[7], VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO,	out_data[17]),
                // Softmax
                vxSoftmaxLayer(graph, out_data[17], out), };

        vx_status status = vxVerifyGraph(graph);
        if (status == VX_SUCCESS) {
            status = vxProcessGraph(graph);
        }

        if (status == VX_SUCCESS) {
            for (int i = 0; i < 8; i++)
                if (dump)
                    dumpToFile(out_data[i], i);
            CopyToFromTensor(context, out, output, -1, VX_READ_ONLY);
        }

        for (int n = 0; n < sizeof(cnn_nodes) / sizeof(cnn_nodes[0]); n++) {
            vxReleaseNode(&cnn_nodes[n]);
        }

        vxReleaseGraph(&graph);
    }

    if (in)
        vxReleaseTensor(&in);
    if (out)
        vxReleaseTensor(&out);
    for (int i = 0; i < NUM_OF_LAYERS_OVERFEAT; i++) {
        if (wt_data[i])
            vxReleaseTensor(&wt_data[i]);
        if (bs_data[i])
            vxReleaseTensor(&bs_data[i]);
        if (out_data[i])
            vxReleaseTensor(&out_data[i]);
    }


    if (status == VX_SUCCESS)
        return 0;
    else
        return -1;
}

int Alexnet(int16_t** ppdata, bool raw, int16_t *output) {
    vx_bool dump = vx_true_e;

    // Definitions
    // kernels operate on half the ofms and ifms of a layer
    // except for 3rd conv layer where kernels operateon half ifms and all ofms
    vx_size wt_dims[NUM_OF_LAYERS_ALEXNET][2][6] = { {
            { 11, 11, 3, 96, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } }, // end of first poolingnorm
            { { 5, 5, 48, 128, 0, 0 }, { 5, 5, 48, 128, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } }, // second relu
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },// second norm
            { { 3, 3, 256, 384, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },// third relu
            { { 3, 3, 192, 192, 0, 0 }, { 3, 3, 192, 192, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },// forth relu
            { { 3, 3, 192, 128, 0, 0 }, { 3, 3, 192, 128, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },// fifth relu
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },// third pooling
            { { 9216, 4096, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },//
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },// first fc relu
            { { 4096, 4096, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },
            { { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },// second fc relu
            { { 4096, 1000, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } } };

    pooling_params pool_params = {3, 3, 0,0,VX_NN_DS_SIZE_ROUNDING_FLOOR};
    activation_params relu_params = {VX_NN_ACTIVATION_RELU, 0, 0};
    vx_nn_convolution_params_t conv_params[3] = {{0,0,VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, VX_NN_DS_SIZE_ROUNDING_FLOOR, 0, 0},
            {2,2,VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, VX_NN_DS_SIZE_ROUNDING_FLOOR, 0, 0},
            {1,1,VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, VX_NN_DS_SIZE_ROUNDING_FLOOR, 0, 0}};
    normalization_params norm_params = {VX_NN_NORMALIZATION_ACROSS_MAPS, 5, 0.0001f*64, 0.75f };
    vx_size out_dims[NUM_OF_LAYERS_ALEXNET][6] = {
            { 55, 55, 96, 0, 0, 0 },
            { 55, 55, 96, 0, 0, 0 },
            { 55, 55, 96, 0, 0, 0 },
            {27, 27, 96, 0, 0, 0 }, // ennd of first normpooling
            { 27, 27, 256, 0, 0, 0 },
            { 27, 27, 256, 0, 0, 0 }, // second relu
            { 27, 27, 256, 0, 0, 0 }, // second norm
            { 13, 13, 256, 0, 0, 0 }, // second pooling
            { 13, 13, 384, 0, 0, 0 },
            { 13, 13, 384, 0, 0, 0 }, // third relu
            { 13, 13, 384, 0, 0, 0 },
            { 13, 13, 384, 0, 0, 0 }, // forth relu
            { 13, 13, 256, 0, 0, 0 }, // fifth convolution
            { 13, 13, 256, 0, 0, 0 }, // fifth relu
            { 6, 6, 256, 0, 0, 0 }, // third pooling
            { 4096, 0, 0, 0, 0, 0 },
            { 4096, 0, 0, 0, 0, 0 }, // first fc relu
            { 4096, 0, 0, 0, 0, 0 },
            { 4096, 0, 0, 0, 0, 0 }, // second fc relu
            { 1000, 0, 0, 0, 0, 0 },
            { 1000, 0, 0, 0, 0, 0 },     };


    // Get input
    uint8_t *pInput = (uint8_t *) *ppdata;

    // Get averages
    uint8_t *pAvgR = (uint8_t *) *(ppdata + 1);
    uint8_t *pAvgG = (uint8_t *) *(ppdata + 2);
    uint8_t *pAvgB = (uint8_t *) *(ppdata + 3);

    // Get weights
    int16_t *pInt16Params = (int16_t *) *(ppdata + 4);

    size_t offset = 0;

    // Building network
    // Edges
    vx_status status = VX_SUCCESS;
    vx_context context = vxCreateContext();
    vx_size dim_max = 0;
    vxQueryContext(context, VX_CONTEXT_MAX_TENSOR_DIMS, &dim_max,
            sizeof(dim_max));
    vx_graph graph;
    graph = vxCreateGraph(context);

    void *ptr = NULL;

    vx_tensor in;
    vx_tensor wt_data[NUM_OF_LAYERS_ALEXNET][2] = { 0 };
    vx_tensor bs_data[NUM_OF_LAYERS_ALEXNET][2] = { 0 };
    vx_tensor out_data[NUM_OF_LAYERS_ALEXNET] = { 0 };
    vx_tensor sub_out_data[NUM_OF_LAYERS_ALEXNET][2] = { 0 };

    printf("Building MD Datas\n");

    vx_size sizes[3] = { 227, 227, 3 };
    in = vxCreateTensor(context, 3, sizes, VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);
    if (raw) {
        // if patch was not preprocessed yet - preprocess it
        vx_rectangle_t rect = { 0, 0, 227, 227 };
        vx_object_array inImgs = vxCreateImageObjectArrayFromTensor(in, &rect, 3,
                1, VX_DF_IMAGE_S16);
        preprocessOrigImageAlexnet(context, pInput, pAvgR, pAvgG, pAvgB,
                inImgs);
        vxReleaseObjectArray(&inImgs);
    } else {
        // if received a preprocessed patch - use as is
        CopyToFromTensor(context, in, (int16_t*) pInput, -1,
                VX_WRITE_ONLY);
    }

    int out_dim = 0;
    for (int i = 0; i < NUM_OF_LAYERS_ALEXNET; i++) {
        int wt_dim[2] = { 0 };
        size_t wt_size[2] = { 1, 1 };
        out_dim = 0;
        for (int j = 0; j < dim_max; j++) {
            if (wt_dims[i][0][j] > 0) {
                wt_dim[0]++;
                wt_size[0] *= wt_dims[i][0][j];
            }

            if (wt_dims[i][1][j] > 0) {
                wt_dim[1]++;
                wt_size[1] *= wt_dims[i][1][j];
            }

            if (out_dims[i][j] > 0)
                out_dim++;
        }

        if (wt_dim[0] > 0) {
            wt_data[i][0] = vxCreateTensor(context, wt_dim[0], wt_dims[i][0],
                    VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);
            offset += CopyToFromTensor(context, wt_data[i][0],
                    pInt16Params + offset, wt_size[0]*sizeof(vx_int16), VX_WRITE_ONLY);

            if (wt_dim[1] > 0) {
                wt_data[i][1] = vxCreateTensor(context, wt_dim[1],
                        wt_dims[i][1], VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);
                offset += CopyToFromTensor(context, wt_data[i][1],
                        pInt16Params + offset, wt_size[1]*sizeof(vx_int16), VX_WRITE_ONLY);
            }
            bs_data[i][0] = vxCreateTensor(context, 1,
                    &wt_dims[i][0][wt_dim[0] - 1], VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);
            offset += CopyToFromTensor(context, bs_data[i][0],
                    pInt16Params + offset, wt_dims[i][0][wt_dim[0] - 1]*sizeof(vx_int16),
                    VX_WRITE_ONLY);

            // Fully connected layers have one set of biases
            if ((i < 16) && (wt_dim[1] > 0)) {
                bs_data[i][1] = vxCreateTensor(context, 1,
                        &wt_dims[i][0][wt_dim[0] - 1], VX_TYPE_INT16,
                        Q78_FIXED_POINT_POSITION);
                offset += CopyToFromTensor(context, bs_data[i][1],
                        pInt16Params + offset, wt_dims[i][0][wt_dim[0] - 1]*sizeof(vx_int16),
                        VX_WRITE_ONLY);
            }
        }

//        out_data[i] = vxCreateVirtualTensor(graph, out_dim, out_dims[i],
//                VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);
        out_data[i] = vxCreateTensor(context, out_dim, out_dims[i],
                VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);


        // Building views for "two GPUs" (page 5 http://www.cs.toronto.edu/~fritz/absps/imagenet.pdf)
        vx_size start1[3], end1[3], start2[3], end2[3];
        start1[2] = 0;
        end1[2] = out_dims[i][2] > 2 ? (out_dims[i][2] / 2) : 0;
        start2[2] = end1[2] > 0 ? (end1[2]) : 0;
        end2[2] = out_dims[i][2] > 0 ? (out_dims[i][2]) : 0;
        for (int k = 0; k < 2; k++) {
            start1[k] = start2[k] = 0;
            end1[k] = end2[k] = out_dims[i][k];
        }

        sub_out_data[i][0] = vxCreateTensorFromView(out_data[i], 3, start1, end1);
        sub_out_data[i][1] = vxCreateTensorFromView(out_data[i], 3, start2, end2);

    }

    vx_tensor out = vxCreateTensor(context, out_dim,
            out_dims[NUM_OF_LAYERS_ALEXNET - 1], VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);

    // upscale multiplier
    vx_tensor upscale = vxCreateTensor(context, out_dim, out_dims[NUM_OF_LAYERS_ALEXNET - 2], VX_TYPE_INT16, Q78_FIXED_POINT_POSITION);

    vx_size upscale_size = 1;
    for (int i = 0; i < out_dim; i++)
    {
        upscale_size *= out_dims[NUM_OF_LAYERS_ALEXNET-2][i];
    }
    vx_int16* tmp =  (vx_int16*)calloc (upscale_size, sizeof(vx_int16));
    int16_t Input1 = 256*8;
    for (int i = 0; i < upscale_size; i++)
        memcpy(tmp+i, &Input1, sizeof(Input1));
    CopyToFromTensor(context, upscale, tmp, upscale_size*sizeof(vx_int16), VX_WRITE_ONLY);
    free (tmp);
    dumpToFile(upscale, 0, "upscale");
    vx_scalar mul_scalar;
    vx_float32 scale = 1.0f;
    mul_scalar = vxCreateScalar(context, VX_TYPE_FLOAT32, (void*)&scale);
    status = vxGetStatus((vx_reference)mul_scalar);
    if (status != VX_SUCCESS){
        //abort("cannot cannot create parameter mul_scalar");
        // TODO: fail!
    }

    vx_enum overflowPolicy = VX_CONVERT_POLICY_SATURATE;
    vx_scalar overflowPolicy_scalar = vxCreateScalar(context, VX_TYPE_ENUM, &overflowPolicy);
    vx_enum roundingPolicy = VX_ROUND_POLICY_TO_NEAREST_EVEN;
    vx_scalar roundingPolicy_scalar = vxCreateScalar(context, VX_TYPE_ENUM, &roundingPolicy);

    printf("MD Datas ready\n");
    printf("Building and processing graph.\n");

    for (int p = 0; p < 1; p++) {
        //Nodes
        		if (graph) {
        			vx_node cnn_nodes[] = {
        					// Layer (1) - Conv + Relu, Norm + Pool
        					// Convolution - 3X227X227 --> 96X27X27

        					vxConvolutionLayer(graph, in, wt_data[0][0], bs_data[0][0], &conv_params[0], sizeof(vx_nn_convolution_params_t), out_data[0]),
        					vxActivationLayer(graph, out_data[0], relu_params.function, relu_params.a, relu_params.b,out_data[1]),
                            vxNormalizationLayer(graph, out_data[1], norm_params.type, norm_params.normalization_size, norm_params.alpha, norm_params.beta,
                                    out_data[2]),
        					vxPoolingLayer(graph, out_data[2], VX_NN_POOLING_MAX, pool_params.pooling_size_x, pool_params.pooling_size_y, pool_params.pooling_padding_x,
        					        pool_params.pooling_padding_y, pool_params.rounding, out_data[3]),

        					// Layer (2) - Conv + Relu, Norm + Pool
        					// Convolution - 96*27*27 --> 256*13*13
                            vxConvolutionLayer(graph, sub_out_data[3][0], wt_data[4][0], bs_data[4][0], &conv_params[1], sizeof(vx_nn_convolution_params_t), sub_out_data[4][0]),
                            vxActivationLayer(graph, sub_out_data[4][0], relu_params.function, relu_params.a, relu_params.b,sub_out_data[5][0]),
                            vxConvolutionLayer(graph, sub_out_data[3][1], wt_data[4][1], bs_data[4][1], &conv_params[1], sizeof(vx_nn_convolution_params_t), sub_out_data[4][1]),
                            vxActivationLayer(graph, sub_out_data[4][1], relu_params.function, relu_params.a, relu_params.b,sub_out_data[5][1]),
                            vxNormalizationLayer(graph, out_data[5], norm_params.type, norm_params.normalization_size, norm_params.alpha, norm_params.beta,
                                    out_data[6]),
                            vxPoolingLayer(graph, out_data[6], VX_NN_POOLING_MAX, pool_params.pooling_size_x, pool_params.pooling_size_y, pool_params.pooling_padding_x,
                                    pool_params.pooling_padding_y, pool_params.rounding, out_data[7]),

        					// Layer (3) - Conv + Relu
        					// Convolution - 256X13X13 -> 384X13X13
                            vxConvolutionLayer(graph, out_data[7], wt_data[8][0], bs_data[8][0], &conv_params[2], sizeof(vx_nn_convolution_params_t), out_data[8]),
                            vxActivationLayer(graph, out_data[8], relu_params.function, relu_params.a, relu_params.b,out_data[9]),

        					// Layer (4) - Conv + Relu
        					// Convolution - 384X13X13 -> 384X13X13
                            vxConvolutionLayer(graph, sub_out_data[9][0], wt_data[10][0], bs_data[10][0], &conv_params[2], sizeof(vx_nn_convolution_params_t), sub_out_data[10][0]),
                            vxActivationLayer(graph, sub_out_data[10][0], relu_params.function, relu_params.a, relu_params.b,sub_out_data[11][0]),
                            vxConvolutionLayer(graph, sub_out_data[9][1], wt_data[10][1], bs_data[10][1], &conv_params[2], sizeof(vx_nn_convolution_params_t), sub_out_data[10][1]),
                            vxActivationLayer(graph, sub_out_data[10][1], relu_params.function, relu_params.a, relu_params.b,sub_out_data[11][1]),

        											// Layer (5) - Conv + Relu + MAX
        											// Convolution - 384X13X13 --> 256*6*6
                            vxConvolutionLayer(graph, sub_out_data[11][0], wt_data[12][0], bs_data[12][0], &conv_params[2], sizeof(vx_nn_convolution_params_t), sub_out_data[12][0]),
                            vxActivationLayer(graph, sub_out_data[12][0], relu_params.function, relu_params.a, relu_params.b,sub_out_data[13][0]),
                            vxPoolingLayer(graph, sub_out_data[13][0], VX_NN_POOLING_MAX, pool_params.pooling_size_x, pool_params.pooling_size_y, pool_params.pooling_padding_x,
                                    pool_params.pooling_padding_y, pool_params.rounding, sub_out_data[14][0]),
                            vxConvolutionLayer(graph, sub_out_data[11][1], wt_data[12][1], bs_data[12][1], &conv_params[2], sizeof(vx_nn_convolution_params_t), sub_out_data[12][1]),
                            vxActivationLayer(graph, sub_out_data[12][1], relu_params.function, relu_params.a, relu_params.b,sub_out_data[13][1]),
                            vxPoolingLayer(graph, sub_out_data[13][1], VX_NN_POOLING_MAX, pool_params.pooling_size_x, pool_params.pooling_size_y, pool_params.pooling_padding_x,
                                    pool_params.pooling_padding_y, pool_params.rounding, sub_out_data[14][1]),

        					// Layer (6) - FullyConnected
        					// Convolution - 256*6*6 --> 4096
                            vxFullyConnectedLayer(graph, out_data[14], wt_data[15][0], bs_data[15][0], VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, out_data[15]),
                            vxActivationLayer(graph, out_data[15], relu_params.function, relu_params.a, relu_params.b, out_data[16]),

        					// Layer (7) - FullyConnected 4096 --> 4096
                            vxFullyConnectedLayer(graph, out_data[16], wt_data[17][0], bs_data[17][0], VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, out_data[17]),
                            vxActivationLayer(graph, out_data[17], relu_params.function, relu_params.a, relu_params.b, out_data[18]),

        					// Layer (8) - FullyConnected 4096 --> 1000
        					vxFullyConnectedLayer(graph, out_data[18], wt_data[19][0],
        							bs_data[19][0], VX_CONVERT_POLICY_WRAP, VX_ROUND_POLICY_TO_ZERO, out_data[19]),
        					vxTensorMultiplyNode(graph, out_data[19], upscale, mul_scalar, overflowPolicy, roundingPolicy, out_data[20]),

        					vxSoftmaxLayer(graph, out_data[20], out) };

        			status = vxVerifyGraph(graph);
        			if (status == VX_SUCCESS) {
        				status = vxProcessGraph(graph);
        			}
        			else
        			{
        			    printf ("Graph validation failed!\n");
        			}

        			if (status == VX_SUCCESS) {
        				for (int i = 0; i < NUM_OF_LAYERS_ALEXNET; i++)
        					if (dump)
        					{
        						dumpToFile(out_data[i], i, "output");
                                dumpToFile(wt_data[i][0], i, "weights0");
                                dumpToFile(wt_data[i][1], i, "weights1");
                                dumpToFile(bs_data[i][0], i, "biases0");
                                dumpToFile(bs_data[i][1], i, "biases1");
                                dumpToFile(sub_out_data[i][0], i, "sub_out_data0");
                                dumpToFile(sub_out_data[i][1], i, "sub_out_data1");


        					}

        				if (dump)
        				{
        					dumpToFile(out, NUM_OF_LAYERS_ALEXNET, "output");
                            dumpToFile(in, 0, "input");

        				}

        				CopyToFromTensor(context, out, output, -1, VX_READ_ONLY);
        			}

        			for (int n = 0; n < sizeof(cnn_nodes) / sizeof(cnn_nodes[0]); n++) {
        				vxReleaseNode(&cnn_nodes[n]);
        			}

        			vxReleaseGraph(&graph);
        		}
    }
    if (mul_scalar)
        vxReleaseScalar(&mul_scalar);
    if (in)
        vxReleaseTensor(&in);
    if (out)
        vxReleaseTensor(&out);
    for (int i = 0; i < NUM_OF_LAYERS_ALEXNET; i++) {
        if (wt_data[i][0])
            vxReleaseTensor(&wt_data[i][0]);
        if (wt_data[i][1])
            vxReleaseTensor(&wt_data[i][1]);
        if (bs_data[i][0])
            vxReleaseTensor(&bs_data[i][0]);
        if (bs_data[i][1])
            vxReleaseTensor(&bs_data[i][1]);
        if (out_data[i])
            vxReleaseTensor(&out_data[i]);
    }


    if (status == VX_SUCCESS)
        return 0;
    else
        return -1;
}

/*vx_status inceptionLayerFF(vx_context context, vx_tensor in, vx_enum format,
		vx_uint8 fp_pos, vx_uint32 ifms, vx_uint32 ofm1_11, vx_uint32 ofm2_33t,
		vx_uint32 ofm2_33, vx_uint32 ofm3_33t, vx_uint32 ofm3_33,
		vx_uint32 ofm4, vx_enum pool_type, vx_uint32 stride, vx_tensor out,
		int16_t *pInt16Params, vx_uint32 *offset, int id) {
	vx_bool dump = vx_false_e;
	vx_status status = VX_SUCCESS;
	vx_uint8 dim_max = 0;
	vxQueryContext(context, VX_CONTEXT_MAX_TENSOR_DIMENSIONS, &dim_max,
			sizeof(dim_max));
	vx_size dims[dim_max], dims2[dim_max];
	vx_size outdims[dim_max], indims[dim_max];
	vx_tensor outtmp[4] = { 0 };
	vx_tensor outviews[4] = { 0 };
	vx_tensor weights[8];
	vx_tensor biases[8];
	vx_size i = 0, j = 0, k = 0, num_out_dims;
	vx_uint32 conv_params[4][4] = { { stride, stride, 0, 0 }, { stride, stride,
			1, 1 }, { 1, 1, 0, 0 }, { 1, 1, 1, 1 } };

	vx_array conv_params_array[4] = { vxCreateArray(context, VX_TYPE_UINT32, 4),
			vxCreateArray(context, VX_TYPE_UINT32, 4), vxCreateArray(context,
					VX_TYPE_UINT32, 4), vxCreateArray(context, VX_TYPE_UINT32,
							4) };

	vxAddArrayItems(conv_params_array[0], 4, conv_params[0], 0);
	vxAddArrayItems(conv_params_array[1], 4, conv_params[1], 0);
	vxAddArrayItems(conv_params_array[2], 4, conv_params[2], 0);
	vxAddArrayItems(conv_params_array[3], 4, conv_params[3], 0);

	status |= vxQueryTensor(in, VX_TENSOR_DIMENSIONS, &indims, sizeof(indims));
	status |= vxQueryTensor(out, VX_TENSOR_NUMBER_OF_DIMENSIONS, &num_out_dims,
			sizeof(num_out_dims));
	status |= vxQueryTensor(out, VX_TENSOR_DIMENSIONS, &outdims, sizeof(outdims));

	vx_tensor out_inception = vxCreateTensor(context, num_out_dims, outdims,
			format, fp_pos);

	vx_size start[num_out_dims], end[num_out_dims];
	// output is divided to views along the first dimension
	for (vx_uint32 l = 0; l < num_out_dims; l++) {
		start[l] = 0;
		end[l] = outdims[l];
	}

	end[2] = 0;

	if (ofm1_11 > 0) {
		dims[3] = ofm1_11;
		dims[2] = ifms;
		dims[1] = 1;
		dims[0] = 1;
		end[2] = ofm1_11;
		outviews[j] = vxCreateTensorFromView(out_inception, num_out_dims, start, end);
		weights[i] = vxCreateTensor(context, 4, dims, format, fp_pos);
 *offset += CopyToFromTensor(context, weights[i], pInt16Params + *offset,
				dims[0] * dims[1] * dims[2] * dims[3], VX_WRITE_ONLY);
		biases[i] = vxCreateTensor(context, 1, &dims[3], format, fp_pos);
 *offset += CopyToFromTensor(context, biases[i], pInt16Params + *offset,
				dims[3], VX_WRITE_ONLY);
		status |= vxuIntelCNNConvNonlinearNode(context, in, weights[i],
				biases[i], conv_params_array[0],
				VX_NN_ACTIVATION_RELU,
				NULL, outviews[j]);
		if (dump)
			dumpToFile(weights[i], id, "inc_ofm1_weights");
		if (dump)
			dumpToFile(biases[i], id, "inc_ofm1_biases");
		if (dump)
			dumpToFile(outviews[j], id, "inc_ofm1_out");
		i++;
		j++;
	}

	if ((ofm2_33t > 0) && (ofm2_33 > 0)) {
		dims[3] = ofm2_33t;
		dims[2] = ifms;
		dims[1] = 1;
		dims[0] = 1;
		weights[i] = vxCreateTensor(context, 4, dims, format, fp_pos);
 *offset += CopyToFromTensor(context, weights[i], pInt16Params + *offset,
				dims[0] * dims[1] * dims[2] * dims[3], VX_WRITE_ONLY);
		biases[i] = vxCreateTensor(context, 1, &dims[3], format, fp_pos);
 *offset += CopyToFromTensor(context, biases[i], pInt16Params + *offset,
				dims[3], VX_WRITE_ONLY);
		dims[2] = ofm2_33t;
		dims[1] = (indims[1] - dims[1]) + 1;
		dims[0] = (indims[0] - dims[0]) + 1;
		outtmp[k] = vxCreateTensor(context, 3, dims, format, fp_pos);
		status |= vxuIntelCNNConvNonlinearNode(context, in, weights[i],
				biases[i], conv_params_array[2],
				VX_NN_ACTIVATION_RELU,
				NULL, outtmp[k]);
		if (dump)
			dumpToFile(weights[i], id, "inc_ofm2a_weights");
		if (dump)
			dumpToFile(biases[i], id, "inc_ofm2a_biases");
		if (dump)
			dumpToFile(outtmp[k], id, "inc_ofm2a_out");
		i++;
		k++;

		dims[3] = ofm2_33;
		dims[2] = ofm2_33t;
		dims[1] = 3;
		dims[0] = 3;
		start[2] = end[2];
		end[2] = start[2] + ofm2_33;
		outviews[j] = vxCreateTensorFromView(out_inception, num_out_dims, start, end);
		weights[i] = vxCreateTensor(context, 4, dims, format, fp_pos);
 *offset += CopyToFromTensor(context, weights[i], pInt16Params + *offset,
				dims[0] * dims[1] * dims[2] * dims[3], VX_WRITE_ONLY);
		biases[i] = vxCreateTensor(context, 1, &dims[3], format, fp_pos);
 *offset += CopyToFromTensor(context, biases[i], pInt16Params + *offset,
				dims[3], VX_WRITE_ONLY);
		status |= vxuIntelCNNConvNonlinearNode(context, outtmp[k - 1],
				weights[i], biases[i], conv_params_array[1],
				VX_NN_ACTIVATION_RELU, NULL, outviews[j]);
		if (dump)
			dumpToFile(weights[i], id, "inc_ofm2_weights");
		if (dump)
			dumpToFile(biases[i], id, "inc_ofm2_biases");
		if (dump)
			dumpToFile(outviews[j], id, "inc_ofm2_out");
		i++;
		j++;
	}

	if ((ofm3_33t > 0) && (ofm3_33 > 0)) {
		dims[3] = ofm3_33t;
		dims[2] = ifms;
		dims[1] = 1;
		dims[0] = 1;
		weights[i] = vxCreateTensor(context, 4, dims, format, fp_pos);
 *offset += CopyToFromTensor(context, weights[i], pInt16Params + *offset,
				dims[0] * dims[1] * dims[2] * dims[3], VX_WRITE_ONLY);
		biases[i] = vxCreateTensor(context, 1, &dims[3], format, fp_pos);
 *offset += CopyToFromTensor(context, biases[i], pInt16Params + *offset,
				dims[3], VX_WRITE_ONLY);
		dims2[2] = ofm3_33t;
		dims2[1] = (indims[1] - dims[1]) + 1;
		dims2[0] = (indims[0] - dims[0]) + 1;
		outtmp[k] = vxCreateTensor(context, 3, dims2, format, fp_pos);
		status |= vxuIntelCNNConvNonlinearNode(context, in, weights[i],
				biases[i], conv_params_array[2],
				VX_NN_ACTIVATION_RELU,
				NULL, outtmp[k]);
		if (dump)
			dumpToFile(weights[i], id, "inc_ofm3a_weights");
		if (dump)
			dumpToFile(biases[i], id, "inc_ofm3a_biases");
		if (dump)
			dumpToFile(outtmp[k], id, "inc_ofm3a_out");
		i++;
		k++;

		dims[3] = ofm3_33t;
		dims[2] = ofm3_33t;
		dims[1] = 3;
		dims[0] = 3;
		weights[i] = vxCreateTensor(context, 4, dims, format, fp_pos);
 *offset += CopyToFromTensor(context, weights[i], pInt16Params + *offset,
				dims[0] * dims[1] * dims[2] * dims[3], VX_WRITE_ONLY);
		biases[i] = vxCreateTensor(context, 1, &dims[3], format, fp_pos);
 *offset += CopyToFromTensor(context, biases[i], pInt16Params + *offset,
				dims[3], VX_WRITE_ONLY);
		dims[2] = ofm3_33t;
		dims[1] = (dims2[1] + 2 - dims[1]) + 1;
		dims[0] = (dims2[0] + 2 - dims[0]) + 1;
		outtmp[k] = vxCreateTensor(context, 3, dims, format, fp_pos);
		status |= vxuIntelCNNConvNonlinearNode(context, outtmp[k - 1],
				weights[i], biases[i], conv_params_array[3],
				VX_NN_ACTIVATION_RELU, NULL, outtmp[k]);
		if (dump)
			dumpToFile(weights[i], id, "inc_ofm3b_weights");
		if (dump)
			dumpToFile(biases[i], id, "inc_ofm3b_biases");
		if (dump)
			dumpToFile(outtmp[k], id, "inc_ofm3b_out");
		i++;
		k++;

		dims[3] = ofm3_33;
		dims[2] = ofm3_33t;
		dims[1] = 3;
		dims[0] = 3;
		start[2] = end[2];
		end[2] = start[2] + ofm3_33;
		outviews[j] = vxCreateTensorFromView(out_inception, num_out_dims, start, end);
		weights[i] = vxCreateTensor(context, 4, dims, format, fp_pos);
 *offset += CopyToFromTensor(context, weights[i], pInt16Params + *offset,
				dims[0] * dims[1] * dims[2] * dims[3], VX_WRITE_ONLY);
		biases[i] = vxCreateTensor(context, 1, &dims[3], format, fp_pos);
 *offset += CopyToFromTensor(context, biases[i], pInt16Params + *offset,
				dims[3], VX_WRITE_ONLY);
		status |= vxuIntelCNNConvNonlinearNode(context, outtmp[k - 1],
				weights[i], biases[i], conv_params_array[1],
				VX_NN_ACTIVATION_RELU, NULL, outviews[j]);
		if (dump)
			dumpToFile(weights[i], id, "inc_ofm3_weights");
		if (dump)
			dumpToFile(biases[i], id, "inc_ofm3_biases");
		if (dump)
			dumpToFile(outviews[j], id, "inc_ofm3_out");
		i++;
		j++;
	}

	vx_size pool_params[6] = { 3, 3, 1, 1, stride, stride };
	vx_array pool_params_array = vxCreateArray(context, VX_TYPE_SIZE, 6);
	vxAddArrayItems(pool_params_array, 6, pool_params, 0);

	if (ofm4 > 0) {
		dims[2] = ifms;
		dims[1] = (indims[1] + 2 * 1 - dims[1]) / stride + 1;
		dims[0] = (indims[0] + 2 * 1 - dims[0]) / stride + 1;
		outtmp[k] = vxCreateTensor(context, 3, dims, format, fp_pos);
		status |= vxuIntelCNNPoolingNode(context, in, pool_type,
				pool_params_array, outtmp[k]);
		k++;

		dims[3] = ofm4;
		dims[2] = ifms;
		dims[1] = 1;
		dims[0] = 1;
		start[2] = end[2];
		end[2] = start[2] + ofm4;
		outviews[j] = vxCreateTensorFromView(out_inception, num_out_dims, start, end);
		weights[i] = vxCreateTensor(context, 4, dims, format, fp_pos);
 *offset += CopyToFromTensor(context, weights[i], pInt16Params + *offset,
				dims[0] * dims[1] * dims[2] * dims[3], VX_WRITE_ONLY);
		biases[i] = vxCreateTensor(context, 1, &dims[3], format, fp_pos);
 *offset += CopyToFromTensor(context, biases[i], pInt16Params + *offset,
				dims[3], VX_WRITE_ONLY);
		status |= vxuIntelCNNConvNonlinearNode(context, outtmp[k - 1],
				weights[i], biases[i], conv_params_array[2],
				VX_NN_ACTIVATION_RELU, NULL, outviews[j]);
		if (dump)
			dumpToFile(weights[i], id, "inc_ofm4_weights");
		if (dump)
			dumpToFile(biases[i], id, "inc_ofm4_biases");
		if (dump)
			dumpToFile(outviews[j], id, "inc_ofm4_out");
		i++;
		j++;
	} else {
		start[2] = end[2];
		end[2] = start[2] + ifms;
		outviews[j] = vxCreateTensorFromView(out_inception, num_out_dims, start, end);
		status |= vxuIntelCNNPoolingNode(context, in, pool_type,
				pool_params_array, outviews[j]);
		j++;
	}

	status |= vxuIntelCNNNonlinearNode(context, out_inception,
			VX_NN_ACTIVATION_RELU, NULL, out);

	if (conv_params_array[0])
		vxReleaseArray(&conv_params_array[0]);
	if (conv_params_array[1])
		vxReleaseArray(&conv_params_array[1]);
	if (conv_params_array[2])
		vxReleaseArray(&conv_params_array[2]);
	if (conv_params_array[3])
		vxReleaseArray(&conv_params_array[3]);
	if (pool_params_array)
		vxReleaseArray(&pool_params_array);

	for (vx_uint32 ii = 0; ii < i; ii++) {
		if (weights[ii] != 0) {
			vxReleaseTensor(&weights[ii]);
		}
		if (biases[ii] != 0) {
			vxReleaseTensor(&biases[ii]);
		}
	}

	for (vx_uint32 jj = 0; jj < j; jj++) {
		if (outviews[jj] != 0) {
			vxReleaseTensor(&outviews[jj]);
		}
	}

	for (vx_uint32 kk = 0; kk < k; kk++) {
		if (outtmp[kk] != 0) {
			vxReleaseTensor(&outtmp[kk]);
		}
	}

	vxReleaseTensor(&out_inception);

	return status;
}*/

int Googlenet2(int16_t** ppdata, vx_enum format, vx_uint8 fp_pos, bool raw,
        bool immediate, int16_t *output) {
    /*	vx_bool dump = vx_false_e;

	vx_size wt_dims[NUM_OF_LAYERS_GOOGLENET2][6] = { { 7, 7, 3, 64, 0, 0 }, { 3,
			3, 64, 192, 0, 0 },
			// Inception layers here
			{ 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 }, {
					0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 },
					{ 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 }, {
							0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 }, { 1, 1, 1024,
									1000, 0, 0 } };

	vx_size out_dims[NUM_OF_LAYERS_GOOGLENET2][6] = { { 56, 56, 64, 0, 0, 0 }, {
			28, 28, 192, 0, 0, 0 }, { 28, 28, 256, 0, 0, 0 }, { 28, 28, 320, 0,
					0, 0 }, { 14, 14, 576, 0, 0, 0 }, { 14, 14, 576, 0, 0, 0 }, { 14,
							14, 576, 0, 0, 0 }, { 14, 14, 576, 0, 0, 0 },
							{ 14, 14, 576, 0, 0, 0 }, { 7, 7, 1024, 0, 0, 0 }, { 7, 7, 1024, 0,
									0, 0 }, { 7, 7, 1024, 0, 0, 0 }, { 1000, 0, 0, 0, 0, 0 }, };

	vx_tensor wt_data[NUM_OF_LAYERS_GOOGLENET2] = { 0 };
	vx_tensor bs_data[NUM_OF_LAYERS_GOOGLENET2] = { 0 };
	vx_tensor out_data[NUM_OF_LAYERS_GOOGLENET2] = { 0 };

	// Get input
	uint8_t *pInput = *(uint8_t**) ppdata;

	// Get weights
	int16_t *pInt16Params = (int16_t *) *(ppdata + 1);

	vx_uint32 offset = 0;

	// Building googlenet2 network
	// Edges
	vx_status status = VX_SUCCESS;
	vx_context context = vxCreateContext();
	vx_uint8 dim_max = 0;
	vxQueryContext(context, VX_CONTEXT_MAX_TENSOR_DIMENSIONS, &dim_max,
			sizeof(dim_max));
	vx_graph graph;
	if (!immediate) {
		graph = vxCreateGraph(context);
	}

	void *ptr = NULL;

	printf("Building MD Datas!\n");

	vx_size sizes[3] = { 224, 224, 3 };
	vx_tensor in = vxCreateTensor(context, 3, sizes, format, fp_pos);
	if (raw) {
		// if patch was not preprocessed yet - preprocess it
		if (format == VX_TYPE_FLOAT16) {
			preprocessImageNetF16(context, pInput, in);
		} else {
			vx_rectangle_t rect = { 0, 0, 224, 224 };
			vx_object_array inImgs = vxCreateImageObjectArrayFromTensor(in,
					rect, 3, 1, VX_DF_IMAGE_S16);
			preprocessImageNetQ78(context, pInput, 224, inImgs);
			vxReleaseObjectArray(&inImgs);
		}
	} else {
		status |= CopyToFromTensor(context, in, (int16_t*) pInput, -1,
				VX_WRITE_ONLY);
	}

	vx_size start[3], end[3];

	start[0] = 0;    // 16;
	end[0] = 224;    // 240;
	start[1] = 0;    // 16;
	end[1] = 224;    // 240;
	start[2] = 0;
	end[2] = 3; // A single RGB image => 3 vx_image

	vx_tensor patch = vxCreateTensorFromView(in, 3, start, end);

	printf("MD Datas ready\n");
	printf("Building and processing graph. Immediate=%d\n", immediate);

	vx_size i = 0;
	vx_size num_of_dims = 0;

	for (i = 0; i < NUM_OF_LAYERS_GOOGLENET2 - 1; i++) {
		num_of_dims = 0;
		out_data[i] = vxCreateTensor(context, 3, out_dims[i], format, fp_pos);

		if (wt_dims[i][0] > 0) {
			int wt_size = 1;
			for (int j = 0; j < dim_max; j++) {
				if (wt_dims[i][j] > 0) {
					num_of_dims++;
					wt_size *= wt_dims[i][j];
				}
			}

			wt_data[i] = vxCreateTensor(context, 4, wt_dims[i], format, fp_pos);
			offset += CopyToFromTensor(context, wt_data[i],
					pInt16Params + offset, wt_size, VX_WRITE_ONLY);
			bs_data[i] = vxCreateTensor(context, 1,
					&wt_dims[i][num_of_dims - 1], format, fp_pos);
			offset += CopyToFromTensor(context, bs_data[i],
					pInt16Params + offset, wt_dims[i][3], VX_WRITE_ONLY);
		}
	}

	i = 0;

	vx_uint32 conv_params[3][4] = { { 2, 2, 3, 3 }, { 1, 1, 1, 1 },
			{ 1, 1, 0, 0 } };

	vx_array conv_params_array[3] = { vxCreateArray(context, VX_TYPE_UINT32, 4),
			vxCreateArray(context, VX_TYPE_UINT32, 4), vxCreateArray(context,
					VX_TYPE_UINT32, 4) };

	vxAddArrayItems(conv_params_array[0], 4, conv_params[0], 0);
	vxAddArrayItems(conv_params_array[1], 4, conv_params[1], 0);
	vxAddArrayItems(conv_params_array[2], 4, conv_params[2], 0);

	vx_size pool_params[2][6] = { { 3, 3, 0, 0, 2, 2 }, { 7, 7, 0, 0, 1, 1 } };
	vx_array pool_params_array[2] = { vxCreateArray(context, VX_TYPE_SIZE, 6),
			vxCreateArray(context, VX_TYPE_SIZE, 6) };

	vxAddArrayItems(pool_params_array[0], 6, pool_params[0], 0);
	vxAddArrayItems(pool_params_array[1], 6, pool_params[1], 0);

	if (dump)
		dumpToFile(in, 0, "in");

	// (1) 3X224X224 --> 64X56X56
	status |= vxuIntelCNNConvNonlinearPoolingNode(context, patch, wt_data[i],
			bs_data[i], conv_params_array[0],
			VX_NN_ACTIVATION_RELU, NULL,
			VX_NN_POOLING_MAX, pool_params_array[0],
			out_data[i]);
	;
	if (dump) {
		dumpToFile(wt_data[i], i, "weights");
		dumpToFile(bs_data[i], i, "biases");
		dumpToFile(out_data[i], i, "out");
	}
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (2) 64X56X56 --> 192x28x28
	status |= vxuIntelCNNConvNonlinearPoolingNode(context, out_data[i - 1],
			wt_data[i], bs_data[i], conv_params_array[1],
			VX_NN_ACTIVATION_RELU, NULL,
			VX_NN_POOLING_MAX, pool_params_array[0],
			out_data[i]);
	;
	if (dump) {
		dumpToFile(wt_data[i], i, "weights");
		dumpToFile(bs_data[i], i, "biases");
		dumpToFile(out_data[i], i, "out");
	}
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (3a) 192x28x28 -> 256x28x28
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 192, 64, 64,
			64, 64, 96, 32, VX_NN_POOLING_AVG, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (3b) 256x28x28 -> 320x28x28
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 256, 64, 64,
			96, 64, 96, 64, VX_NN_POOLING_AVG, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (3c) 320x28x28 -> 576x14x14
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 320, 0, 128,
			160, 64, 96, 0, VX_NN_POOLING_MAX, 2,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (4a) 576x14x14 -> 576x14x14
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 576, 224, 64,
			96, 96, 128, 128, VX_NN_POOLING_AVG, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (4b) 576x14x14 -> 576x14x14
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 576, 192, 96,
			128, 96, 128, 128, VX_NN_POOLING_AVG, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (4c) 576x14x14 -> 576x14x14
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 576, 160, 128,
			160, 128, 160, 96, VX_NN_POOLING_AVG, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (4d) 576x14x14 -> 576x14x14
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 576, 96, 128,
			192, 160, 192, 96, VX_NN_POOLING_AVG, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (4e) 576x14x14 -> 1024x7x7
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 576, 0, 128,
			192, 192, 256, 0, VX_NN_POOLING_MAX, 2,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (5a) 1024x7x7 -> 1024x7x7
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 1024, 352, 192,
			320, 160, 224, 128, VX_NN_POOLING_AVG, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	// (5b) 1024x7x7 -> 1024x7x7
	status |= inceptionLayerFF(context, out_data[i - 1], format, fp_pos, 1024, 352, 192,
			320, 192, 224, 128, VX_NN_POOLING_MAX, 1,
			out_data[i], pInt16Params, &offset, i);
	if (dump)
		dumpToFile(out_data[i], i, "out");
	printf("Layer %d completed, status=%d\n", ++i, status);

	out_data[NUM_OF_LAYERS_GOOGLENET2 - 1] = vxCreateTensor(context, 1,
			out_dims[NUM_OF_LAYERS_GOOGLENET2 - 1], format, fp_pos);

	int wt_size = 1;
	num_of_dims = 0;
	for (int j = 0; j < dim_max; j++) {
		if (wt_dims[NUM_OF_LAYERS_GOOGLENET2 - 1][j] > 0) {
			num_of_dims++;
			wt_size *= wt_dims[NUM_OF_LAYERS_GOOGLENET2 - 1][j];
		}
	}

	wt_data[NUM_OF_LAYERS_GOOGLENET2 - 1] = vxCreateTensor(context, 4,
			wt_dims[NUM_OF_LAYERS_GOOGLENET2 - 1], format, fp_pos);
	offset += CopyToFromTensor(context, wt_data[NUM_OF_LAYERS_GOOGLENET2 - 1],
			pInt16Params + offset, wt_size, VX_WRITE_ONLY);

	bs_data[NUM_OF_LAYERS_GOOGLENET2 - 1] = vxCreateTensor(context, 1,
			&wt_dims[NUM_OF_LAYERS_GOOGLENET2 - 1][num_of_dims - 1], format,
			fp_pos);
	offset += CopyToFromTensor(context, bs_data[NUM_OF_LAYERS_GOOGLENET2 - 1],
			pInt16Params + offset,
			wt_dims[NUM_OF_LAYERS_GOOGLENET2 - 1][num_of_dims - 1],
			VX_WRITE_ONLY);

	out_data[NUM_OF_LAYERS_GOOGLENET2 - 1] = vxCreateTensor(context, 1,
			out_dims[NUM_OF_LAYERS_GOOGLENET2 - 1], format, fp_pos);
	vx_tensor out = vxCreateTensor(context, 1,
			out_dims[NUM_OF_LAYERS_GOOGLENET2 - 1], format, fp_pos);

	// (6) 1024X7X7 --> 1000X1X1
	status |= vxuIntelCNNConvPoolingNode(context, out_data[i - 1], wt_data[i],
			bs_data[i], conv_params_array[2],
			VX_NN_POOLING_AVG, pool_params_array[1],
			out_data[i]);
	;
	if (dump) {
		dumpToFile(wt_data[i], i, "weights");
		dumpToFile(bs_data[i], i, "biases");
		dumpToFile(out_data[i], i, "out");
	}
	printf("Layer %d completed, status=%d\n", ++i, status);

	status |= vxuIntelCNNSoftmaxNode(context, out_data[i - 1], out);
	if (dump)
		dumpToFile(out, i);
	CopyToFromTensor(context, out, output, -1, VX_READ_ONLY);

	if (in)
		vxReleaseTensor(&in);
	if (out)
		vxReleaseTensor(&out);
	for (int i = 0; i < NUM_OF_LAYERS_GOOGLENET2; i++) {
		if (wt_data[i])
			vxReleaseTensor(&wt_data[i]);
		if (bs_data[i])
			vxReleaseTensor(&bs_data[i]);
		if (out_data[i])
			vxReleaseTensor(&out_data[i]);
	}

	if (conv_params_array[0])
		vxReleaseArray(&conv_params_array[0]);
	if (conv_params_array[1])
		vxReleaseArray(&conv_params_array[1]);
	if (conv_params_array[2])
		vxReleaseArray(&conv_params_array[2]);
	if (pool_params_array[0])
		vxReleaseArray(&pool_params_array[0]);
	if (pool_params_array[1])
		vxReleaseArray(&pool_params_array[1]);

	if (status == VX_SUCCESS)
		return 0;
	else
		return -1;*/
    return -1;
}

