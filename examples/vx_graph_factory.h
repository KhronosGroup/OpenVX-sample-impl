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

#ifndef _VX_EXAMPLE_GRAPH_FACTORY_H_
#define _VX_EXAMPLE_GRAPH_FACTORY_H_

#include <VX/vx.h>
#include <VX/vx_helper.h>

/*! \ingroup group_example */
enum vx_factory_name_e {
    VX_GRAPH_FACTORY_EDGE,
    VX_GRAPH_FACTORY_CORNERS,
    VX_GRAPH_FACTORY_PIPELINE,
};

/*! \brief An prototype of a graph factory method.
 * \ingroup group_example
 */
typedef vx_graph(*vx_graph_factory_f)(vx_context context);

/*! \brief A graph factory structure.
 * \ingroup group_example
 */
typedef struct _vx_graph_factory_t {
    vx_enum factory_name;
    vx_graph_factory_f factory;
} vx_graph_factory_t;

// PROTOTYPES
vx_graph vxEdgeGraphFactory(vx_context context);
vx_graph vxCornersGraphFactory(vx_context context);
vx_graph vxPipelineGraphFactory(vx_context context);

#endif
