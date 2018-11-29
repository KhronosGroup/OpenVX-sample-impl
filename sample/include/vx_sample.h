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

#ifndef _VX_SAMPLE_H_
#define _VX_SAMPLE_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The Sample Extensions (which may end up in later revisions)
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 */

enum _vx_sample_kernels_e {
    VX_KERNEL_SAMPLE_BASE = VX_KERNEL_BASE(VX_ID_KHRONOS, 0),

    /*! \brief */
    VX_KERNEL_SAMPLE_CHILD_GRAPH,
};

/*! \brief Creates a child graph within a node of a parent graph.
 * \param [in] parent The parent graph.
 * \param [in] child The child graph.
 * \note Child graphs do not have a strictly defined data interface like
 * kernels and as such, lack a "signature". However, data does go in and out
 * like a kernel. When a graph is imported as a node into a parent, a parameter
 * list is generated which contains the data objects which are singularly referenced
 * as VX_INPUT or VX_OUTPUT or VX_BIDIRECTION
 * (there can be only 1 referent in the total graph here too). The parameter list
 * order is first all inputs, then bi-directional, then outputs, by order of which nodes
 * where declared in the child graph.
 * \ingroup group_int_graph
 */
vx_node ownCreateNodeFromGraph(vx_graph parent, vx_graph child);

#endif /* _VX_SAMPLE_H_ */

