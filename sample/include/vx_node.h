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

#ifndef _OPENVX_INT_NODE_H_
#define _OPENVX_INT_NODE_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The internal node implementation.
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_node Internal Node API
 * \ingroup group_internal
 * \brief The internal Node API.
 */

/*! \brief The internal node attributes
 * \ingroup group_int_node
 */
enum vx_node_attribute_internal_e {
    /*\brief The attribute used to get the pointer to tile local memory */
    VX_NODE_ATTRIBUTE_TILE_MEMORY_PTR = VX_ATTRIBUTE_BASE(VX_ID_KHRONOS, VX_TYPE_NODE) + 0xD,
};

/*! \brief Node parameter setter, no check.
 * \ingroup group_int_node
 */
void ownNodeSetParameter(vx_node node, vx_uint32 index, vx_reference value);

 /*! \brief Used to print the values of the node.
  * \ingroup group_int_node
  */
void ownPrintNode(vx_node node);

/*! \brief Used to completely destroy a node.
 * \ingroup group_int_node
 */
void ownDestructNode(vx_reference ref);

/*! \brief Used to remove a node from a graph.
 * \ingroup group_int_node
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors.
 * \retval VX_ERROR_INVALID_REFERENCE If *n is not a <tt>\ref vx_node</tt>.
 */
vx_status ownRemoveNodeInt(vx_node *n);

/*! \brief Used to set the graph as a child of the node within another graph.
 * \param [in] n The node.
 * \param [in] g The child graph.
 * \retval VX_ERROR_INVALID_GRAPH The Graph's parameters do not match the Node's
 * parameters.
 * \ingroup group_int_node
 */
vx_status ownSetChildGraphOfNode(vx_node node, vx_graph graph);

/*! \brief Retrieves the handle of the child graph, if it exists.
 * \param [in] n The node.
 * \return Returns the handle of the child graph or zero if it doesn't have one.
 * \ingroup group_int_node
 */
vx_graph ownGetChildGraphOfNode(vx_node node);

#endif
