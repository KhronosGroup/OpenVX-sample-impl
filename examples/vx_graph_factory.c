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
 * \brief The example main code.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vx_graph_factory.h>

/*! \brief The array of factory functions.
 * \ingroup group_example
 */
vx_graph_factory_t factories[] = {
    {VX_GRAPH_FACTORY_EDGE, vxEdgeGraphFactory},
    {VX_GRAPH_FACTORY_CORNERS, vxCornersGraphFactory},
    {VX_GRAPH_FACTORY_PIPELINE, vxPipelineGraphFactory},
};

/*! \brief Returns a graph from one of the matching factories.
 * \ingroup group_example
 */
vx_graph vxGraphFactory(vx_context context, vx_enum factory_name)
{
    vx_uint32 f = 0;
    for (f = 0; f < dimof(factories); f++)
    {
        if (factory_name == factories[f].factory_name)
        {
            return factories[f].factory(context);
        }
    }
    return 0;
}

/*! \brief The graph factory example.
 * \ingroup group_example
 */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    vx_status status = VX_SUCCESS;
    vx_context context = vxCreateContext();
    if (vxGetStatus((vx_reference)context) == VX_SUCCESS)
    {
        vx_image images[] = {
                vxCreateImage(context, 640, 480, VX_DF_IMAGE_U8),
                vxCreateImage(context, 640, 480, VX_DF_IMAGE_S16),
        };
        vx_graph graph = vxGraphFactory(context, VX_GRAPH_FACTORY_EDGE);
        if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
        {
            vx_uint32 p, num = 0;
            status |= vxQueryGraph(graph, VX_GRAPH_NUMPARAMETERS, &num, sizeof(num));
            if (status == VX_SUCCESS)
            {
                printf("There are %u parameters to this graph!\n", num);
                for (p = 0; p < num; p++)
                {
                    vx_parameter param = vxGetGraphParameterByIndex(graph, p);
                    if (param)
                    {
                        vx_enum dir = 0;
                        vx_enum type = 0;
                        status |= vxQueryParameter(param, VX_PARAMETER_DIRECTION, &dir, sizeof(dir));
                        status |= vxQueryParameter(param, VX_PARAMETER_TYPE, &type, sizeof(type));
                        printf("graph.parameter[%u] dir:%d type:%08x\n", p, dir, type);
                        vxReleaseParameter(&param);
                    }
                    else
                    {
                        printf("Invalid parameter retrieved from graph!\n");
                    }
                }

                status |= vxSetGraphParameterByIndex(graph, 0, (vx_reference)images[0]);
                status |= vxSetGraphParameterByIndex(graph, 1, (vx_reference)images[1]);
            }

            status |= vxVerifyGraph(graph);
            if (status == VX_SUCCESS)
            {
                status = vxProcessGraph(graph);
                if (status == VX_SUCCESS)
                {
                    printf("Ran Graph!\n");
                }
            }
            vxReleaseGraph(&graph);
        }
        else
        {
            printf("Failed to create graph!\n");
        }
        vxReleaseContext(&context);
    }
    else
    {
        printf("failed to create context!\n");
    }
    return status;
}

