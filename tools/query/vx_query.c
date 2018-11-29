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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VX/vx.h>
#include <VX/vx_helper.h>

typedef struct _vx_type_name_t {
    vx_char name[128];
    vx_enum tenum;
    vx_enum type;
    vx_size size;
} vx_type_name_t;

#define _STR2(x) #x,(vx_enum)x

/*!< The list of settable attributes on a kernel */
vx_type_name_t attribute_names[] = {
    {_STR2(VX_KERNEL_LOCAL_DATA_SIZE),  VX_TYPE_SIZE, sizeof(vx_size)},
};

vx_type_name_t parameter_names[] = {
    {_STR2(VX_TYPE_INVALID), VX_TYPE_ENUM, sizeof(vx_enum)},
    /* SCALARS */
    {_STR2(VX_TYPE_CHAR), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_INT8), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_UINT8), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_INT16), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_UINT16), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_INT32), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_UINT32), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_INT64), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_UINT64), VX_TYPE_ENUM, sizeof(vx_enum)},
#if defined(EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT)
    {_STR2(VX_TYPE_FLOAT16), VX_TYPE_ENUM, sizeof(vx_enum)},
#endif
    {_STR2(VX_TYPE_FLOAT32), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_FLOAT64), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_ENUM), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_SIZE), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_DF_IMAGE), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_SCALAR), VX_TYPE_ENUM, sizeof(vx_enum)},

    /* OBJECTS */
    {_STR2(VX_TYPE_REFERENCE), VX_TYPE_ENUM, sizeof(vx_enum)},

    /* FRAMEWORK OBJECTS */
    {_STR2(VX_TYPE_CONTEXT), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_GRAPH), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_NODE), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_KERNEL), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_PARAMETER), VX_TYPE_ENUM, sizeof(vx_enum)},

    /* DATA OBJECTS */
    {_STR2(VX_TYPE_IMAGE), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_RECTANGLE), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_CONVOLUTION), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_PYRAMID), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_THRESHOLD), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_REMAP), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_MATRIX), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_DISTRIBUTION), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_LUT), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_TYPE_ARRAY), VX_TYPE_ENUM, sizeof(vx_enum)},

    /* IMAGES */
//  {_STR2(VX_DF_IMAGE_VIRT)},
    {_STR2(VX_DF_IMAGE_NV12), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_NV21), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_RGB), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_RGBX), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_UYVY), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_YUYV), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_IYUV), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_YUV4), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_U8), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_U16), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_S16), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_U32), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {_STR2(VX_DF_IMAGE_S32), VX_TYPE_DF_IMAGE, sizeof(vx_df_image)},
    {"UNKNOWN", 0, VX_TYPE_ENUM, sizeof(vx_enum)}
};

vx_type_name_t direction_names[] = {
    {_STR2(VX_INPUT), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_OUTPUT), VX_TYPE_ENUM, sizeof(vx_enum)},
    {_STR2(VX_BIDIRECTIONAL), VX_TYPE_ENUM, sizeof(vx_enum)},
};

static void vxPrintAllLog(vx_context context)
{
    vx_char entry[VX_MAX_LOG_MESSAGE_LEN];
    vx_status status = VX_SUCCESS;
    do
    {
        status = vxGetLogEntry((vx_reference)context, entry);
        if (status != VX_SUCCESS)
        {
            printf("[%d] %s", status, entry);
        }
    } while (status != VX_SUCCESS);
}

int main(int argc, char *argv[])
{
    vx_status status = VX_SUCCESS;
    vx_context context = vxCreateContext();
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_char implementation[VX_MAX_IMPLEMENTATION_NAME];
        vx_char *extensions = NULL;
        vx_int32 m, modules = 0;
        vx_uint32 k, kernels = 0;
        vx_uint32 p, parameters = 0;
        vx_uint32 a = 0;
        vx_uint16 vendor, version;
        vx_size size = 0;
        vx_kernel_info_t *table = NULL;

        // take each arg as a module name to load
        for (m = 1; m < argc; m++)
        {
            if (vxLoadKernels(context, argv[m]) != VX_SUCCESS)
                printf("Failed to load module %s\n", argv[m]);
            else
                printf("Loaded module %s\n", argv[m]);
        }

        vxPrintAllLog(context);
        vxRegisterHelperAsLogReader(context);
        vxQueryContext(context, VX_CONTEXT_VENDOR_ID, &vendor, sizeof(vendor));
        vxQueryContext(context, VX_CONTEXT_VERSION, &version, sizeof(version));
        vxQueryContext(context, VX_CONTEXT_IMPLEMENTATION, implementation, sizeof(implementation));
        vxQueryContext(context, VX_CONTEXT_MODULES, &modules, sizeof(modules));
        vxQueryContext(context, VX_CONTEXT_EXTENSIONS_SIZE, &size, sizeof(size));
        printf("implementation=%s (%02x:%02x) has %u modules\n", implementation, vendor, version, modules);
        extensions = malloc(size);
        if (extensions)
        {
            vx_char *line = extensions, *token = NULL;
            vxQueryContext(context, VX_CONTEXT_EXTENSIONS, extensions, size);
            do {
                token = strtok(line, " ");
                if (token)
                    printf("extension: %s\n", token);
                line = NULL;
            } while (token);
            free(extensions);
        }
        status = vxQueryContext(context, VX_CONTEXT_UNIQUE_KERNELS, &kernels, sizeof(kernels));
        if (status != VX_SUCCESS) goto exit;
        printf("There are %u kernels\n", kernels);
        size = kernels * sizeof(vx_kernel_info_t);
        table = malloc(size);
        status = vxQueryContext(context, VX_CONTEXT_UNIQUE_KERNEL_TABLE, table, size);
        for (k = 0; k < kernels && table != NULL && status == VX_SUCCESS; k++)
        {
            vx_kernel kernel = vxGetKernelByEnum(context, table[k].enumeration);
            if (kernel && vxGetStatus((vx_reference)kernel) == VX_SUCCESS)
            {
                status = vxQueryKernel(kernel, VX_KERNEL_PARAMETERS, &parameters, sizeof(parameters));
                printf("\t\tkernel[%u]=%s has %u parameters (%d)\n",
                        table[k].enumeration,
                        table[k].name,
                        parameters,
                        status);
                for (p = 0; p < parameters; p++)
                {
                    vx_uint32 tIdx;
                    vx_uint32 dIdx;
                    vx_enum type = VX_TYPE_INVALID;
                    vx_enum dir = VX_INPUT;

                    vx_parameter parameter = vxGetKernelParameterByIndex(kernel, p);

                    status = VX_SUCCESS;
                    status |= vxQueryParameter(parameter, VX_PARAMETER_TYPE, &type, sizeof(type));
                    status |= vxQueryParameter(parameter, VX_PARAMETER_DIRECTION, &dir, sizeof(dir));
                    for (tIdx = 0; tIdx < dimof(parameter_names); tIdx++)
                        if (parameter_names[tIdx].tenum == type)
                            break;
                    for (dIdx = 0; dIdx < dimof(direction_names); dIdx++)
                        if (direction_names[dIdx].tenum == dir)
                            break;
                    if (status == VX_SUCCESS)
                        printf("\t\t\tparameter[%u] type:%s dir:%s\n", p,
                            parameter_names[tIdx].name,
                            direction_names[dIdx].name);
                    vxReleaseParameter(&parameter);
                }
                for (a = 0; a < dimof(attribute_names); a++)
                {
                    switch (attribute_names[a].type)
                    {
                        case VX_TYPE_SIZE:
                        {
                            vx_size value = 0;
                            if (VX_SUCCESS == vxQueryKernel(kernel, attribute_names[a].tenum, &value, sizeof(value)))
                                printf("\t\t\tattribute[%u] %s = "VX_FMT_SIZE"\n",
                                    attribute_names[a].tenum & VX_ATTRIBUTE_ID_MASK,
                                    attribute_names[a].name,
                                    value);
                            break;
                        }
                        case VX_TYPE_UINT32:
                        {
                            vx_uint32 value = 0;
                            if (VX_SUCCESS == vxQueryKernel(kernel, attribute_names[a].tenum, &value, sizeof(value)))
                                printf("\t\t\tattribute[%u] %s = %u\n",
                                    attribute_names[a].tenum & VX_ATTRIBUTE_ID_MASK,
                                    attribute_names[a].name,
                                    value);
                            break;
                        }
                        default:
                            break;
                    }
                }
                vxReleaseKernel(&kernel);
            }
            else
            {
                printf("ERROR: kernel %s is invalid (%d) !\n", table[k].name, status);
            }
        }

        for (m = 1; m < argc; m++)
        {
            if (vxUnloadKernels(context, argv[m]) != VX_SUCCESS)
                printf("Failed to unload module %s\n", argv[m]);
            else
                printf("Unloaded module %s\n", argv[m]);
        }
exit:
        if (table) free(table);
        vxReleaseContext(&context);
    }
    return 0;
}

