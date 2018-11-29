
# Copyright (c) 2012-2017 The Khronos Group Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


## \file vxCodeWriter.py
## \author Frank Brill <brill@ti.com>


from vxGraph import *

class GraphCodeWriter:
    """
    Class to write the C-code for a given graph to a given filename.
    Indents the code appropriately using its own printLine function.
    """

    def __init__(self, context, graph):
        "Initialized the context name and graph, no indentation."
        self.context = context
        self.graph = graph
        self.printLineIndent = 0

    def indent(self, amount):
        """Increase the level of indentation.  Use a negative amount to
        dedent."""
        self.printLineIndent = self.printLineIndent+amount

    def printLine(self, *args, sep='', end='\n'):
        """Print a given line tot he current output file with the
        correct level of indentation."""
        for i in range(0, self.printLineIndent):
            print("  ", end='', file=self.outfile)
        print(*args, sep=sep, end=end, file=self.outfile)

    def printNodes(self, nodes, header, printed):
        """Print the C constructors for a list of nodes with the given
        header.  Make note of the fact that each node has been
        constructed."""
        if len(nodes) > 0:
            self.printLine()
            self.printLine("// %s" % header)
        for node in nodes:
            node.printCConstructor(self)
            printed[node] = True

    def sortNodeList(self, nodes, printed):
        """Get a list of nodes and return a sorted version of it such
        that every node in the returned list comes *after* its
        predecessors in the graph that contains them."""
        returnList = []
        while len(nodes) > 0:
            to_idx = 0
            for node in nodes:
                if not node.isReady(printed):
                    nodes[to_idx] = node
                    to_idx += 1
                else:
                    returnList.append(node)
                    printed[node] = True
            del nodes[to_idx:]
        return returnList

    def writeCfile(self, filename):
        """Write the C code to build the given OpenVX graph to the
        given file."""

        # open the output file
        self.graphName, fileext = os.path.splitext(filename)
        try:
            self.outfile = open(filename, "w")
        except:
            print("Failed to open", filename, file=sys.stderr)
            raise

        # print the prologue
        self.printLine('{')
        self.indent(1)
        self.printLine("// The Context")
        self.printLine("vx_context %s = vxCreateContext();" %
                       self.context)
        self.printLine()
        self.printLine("// The Graph")
        self.printLine("vx_graph %s = vxCreateGraph(%s);" %
                       (self.graphName, self.context))

        # separate out the various node types
        printed = dict()
        heads = []
        tails = []
        intermediates = []
        functions = []
        for node in self.graph.nodes.values():
            if isinstance(node, DataNode):
                if len(node.inports[0].links) == 0: heads.append(node)
                elif len(node.outports[0].links) == 0: tails.append(node)
                else: intermediates.append(node)
            else: functions.append(node)

        # print the input data nodes
        self.printNodes(heads, "Inputs", printed)

        # print the intermediate data and links
        self.printNodes(intermediates, "Intermediate data", printed)
        if len(self.graph.links) > 0:
            self.printLine()
            self.printLine("// Links")
        for link in self.graph.links.values():
            link.printCConstructor(self)

        # print the output data nodes
        self.printNodes(tails, "Results", printed)

        # print the functions nodes
        functions = self.sortNodeList(functions, printed)
        self.printNodes(functions, "Functions", printed)

        # print the epilogue
        self.indent(-1)
        self.printLine('}')
