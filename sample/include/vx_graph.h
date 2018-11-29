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

#ifndef _OPENVX_INT_GRAPH_H_
#define _OPENVX_INT_GRAPH_H_

#include <VX/vx.h>

/*!
 * \file
 * \brief The internal graph implementation
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_graph Internal Graph API
 * \ingroup group_internal
 * \brief The Internal Graph API
 */

/*! \brief Finds the first node with the reference as an input.
 * \param [in] graph The graph to traverse
 * \param [in] ref The reference to search for.
 * \param [in] pIndex The pointer to the location to store the index of the first match.
 * \ingroup group_int_graph
 */
vx_status ownFindNodeWithInputReference(vx_graph graph, vx_reference ref, vx_uint32 *pIndex);

/*! \brief Finds the first node with the reference as an input. This is called
 * in the context of execution, not verification.
 * \param [in] graph The graph to traverse
 * \param [in] ref The reference to search for.
 * \param [in] pIndex The pointer to the location to store the index of the first match.
 * \ingroup group_int_graph
 */
vx_status ownFindNodeWithOutBiReference(vx_graph graph, vx_reference ref, vx_uint32 *pIndex);

/*! \brief */
vx_status ownFindNodesWithReference(vx_graph graph,
                                   vx_reference ref,
                                   vx_uint32 refnodes[],
                                   vx_uint32 *count,
                                   vx_enum reftype);

/*! \brief Clears visited flag.
 * \param [in] graph The graph to clear.
 * \ingroup group_int_graph
 */
void ownClearVisitation(vx_graph graph);

/*! \brief Clears execution flag.
 * \param [in] graph The graph to clear.
 * \ingroup group_int_graph
 */
void ownClearExecution(vx_graph graph);

/*! \brief Executes a single node at an index in the graph and captures the performance.
 * \param [in] graph The graph structure.
 * \param [in] index The index of the node to execute.
 * \return Returns the status of the node.
 * \ingroup group_int_graph
 */
vx_status ownExecuteNode(vx_graph graph, vx_uint32 index);
/*!
 * \brief Recursively traverses the graph looking for cycles against the original
 * node.
 * \param [in] graph The graph to traverse.
 * \param [in] parentIndex Originating Node (to check against cycles). We starting
 * from a head node, pass VX_INT_MAX_REF.
 * \param [in] childIndex Subsequent Linked Node Index.
 * \ingroup group_int_graph
 */
vx_status ownTraverseGraph(vx_graph graph,
                          vx_uint32 parentIndex,
                          vx_uint32 childIndex);

/*! \brief Given a set of last nodes, this function will determine the next
 * set of nodes which are capable of being run. Nodes which are encountered but
 * can't be run will be placed in the left nodes list.
 * \param [in] graph The graph structure.
 * \param [in] last_nodes The last list of nodes executed.
 * \param [in] numLast The number of nodes in the last_nodes list which are valid.
 * \param [out] next_nodes The list of nodes next to be executed.
 * \param [in] numNext The number of nodes in the next_nodes list which are valid.
 * \param [out] left_nodes The list of nodes which are next, but can't be executed.
 * \param [in] numLeft The number of nodes in the left_nodes list which are valid.
 * \ingroup group_int_graph
 */
void ownFindNextNodes(vx_graph graph,
                      vx_uint32 last_nodes[VX_INT_MAX_REF], vx_uint32 numLast,
                      vx_uint32 next_nodes[VX_INT_MAX_REF], vx_uint32 *numNext,
                      vx_uint32 left_nodes[VX_INT_MAX_REF], vx_uint32 *numLeft);

/*! \brief This function finds all graph which contain input or bidirectional
 * access to the reference and marks them as unverified.
 * \param [in] ref The reference structure.
 * \ingroup group_int_graph
 */
void ownContaminateGraphs(vx_reference ref);

/*! \brief Destroys a Graph.
 * \ingroup group_int_graph
 */
void ownDestructGraph(vx_reference ref);

#endif
