
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

## \file vxGraph.py
## \author Frank Brill <brill@ti.com>

"""
Implements a graph object that defines an OpenVX processing graph, and
its various sub-components: Node, Port, and Link.  There are
subclasses for DataNodes (which represent blocks of date like images,
buffers, and scalars) and FunctionNodes, which presents OpenVX
processing operations.

Classes defined are:

    Node: A node in the graph.  May represent a data object or a
          function object (there are subclassese to differentiate).  A
          node will have some number of input and output Ports, which
          can be used to link nodes together to establish the dataflow
          of an vision processing algorithm.

    FunctionNode, DataNode: Subclasses of nodes that behave slightly
          differently.

    Port: The input or output ports of nodes.  The image data or other
          data objects flow in and out of these ports.  Ports are
          typed, and two ports may only be linked if their ports have
          the same type.

    Link: Represents a link between an output port of one Node (its
          source, or src), and the input port of another (its
          destination, or dst).  Links between two function nodes will
          have vx 'virtual' images associated with them.

    Graph: The container object that keeps track of all the objects
           (nodes, ports, and links) in the graph.  Every object is
           given an ID and can be retrived via this ID.  The graph can
           validate itself, e.g., to make sure all of its nodes are
           fully defined and have their inputs and outputs connected.
           The graph can be read in from or written out to an XML
           file.
"""

typeTable = { 'A' : 'vx_array', 
              'C' : 'vx_convolution', 
              'D' : 'vx_delay',
              'H' : 'vx_distribution', 
              'I' : 'vx_image', 
              'L' : 'vx_lut', 
              'M' : 'vx_matrix',
              'P' : 'vx_pyramid',
              'R' : 'vx_remap',
              'S' : 'vx_scalar',
              'T' : 'vx_threshold'}
def typeName(datatype):
    return "%s" % (typeTable[datatype][3:].capitalize())

import sys, os
from xml.etree import ElementTree as etree
from collections import OrderedDict
from functools import cmp_to_key
from vxKernel import Kernel

class Node:
    """
    Node objects represent a function or data object in a processing
    graph.  The main data members are the 'nodetype' that indicates
    specifically what function or type of data object the node
    represents, and a list of input and output ports.
    """
    kernelTable = Kernel.getTable()

    def __init__(self, graph, nodetype, nodeid=None):
        "Initialize the basic members and add self to the given graph."

        # Make sure we don't execute twice due to multiple inheritance
        # Note: can't use super() on constructors with different
        # parameters to avoid diamond problem, so we have to do this.
        try:
            testval = self.objecttype # if self.objecttype exists
            return  # bail out because we're already initialized
        except:
            pass # otherwise, go ahead and initialize

        self.objecttype = "node"
        self.nodetype = nodetype
        self.id = nodeid
        self.name = ''
        self.inports = []
        self.outports = []
        graph.addObject(self)

    @classmethod
    def fromTreeElement(cls, graph, n):
        """
        Initialize based on the contents of an elementTree node (from
        an XML file).  Create a DataNode or FunctionNode as
        appropriate for the given type.
        """
        if n.get('type') in Kernel.dataKernels:
            return DataNode(graph, n.get('type'), n.get('id'))
        else: return FunctionNode(graph, n.get('type'), n.get('id'))

    def getKernel(self):
        "Get the kernel object corresponding to this node's type"
        return Node.kernelTable[self.nodetype]

    def addPort(self, port):
        """
        Add a port to the given node.  The ports must be added *in
        order*, so the first input port added becomes input port-0,
        and the next one becomes input port-1 and so on.  Assign ports
        IDs derived from the node ID.
        """
        if port.direction == 'in':
            port.index = len(self.inports)
            port.id = "port-n%si%s" % (self.id[4:], port.index)
            self.inports.append(port)
        elif port.direction == 'out':
            port.index = len(self.outports)
            port.id = "port-n%so%s" % (self.id[4:], port.index)
            self.outports.append(port)
        else:
            print("Invalid port direction:", port.direction, file=sys.stderr)
            exit()
        self.graph.addObject(port) # Is this necessary?
        return self

    def isReady(self, processedNodes):
        """
        Determines whether the node is ready to process based on
        whether its predecessor nodes have been processed.
        """
        for port in self.inports:
            if port.links[0].src.node not in processedNodes: return False
        return True

    def canGetTo(self, node):
        """Determine whether this node can reach the given node by
        recursively following this node's output links.  If True, this
        indicates that linking from the given node to this node would
        create a cycle in the graph (which is illegal)."""
        if self == node: return True
        for port in self.outports:
            for link in port.links:
                if link.dst.node.canGetTo(node): return True
        return False

    def addAttributesToElement(self, elem):
        elem.attrib["id"] = self.id
        elem.attrib["type"] = self.kernel.name
        elem.attrib["x"] = str(self.x)
        elem.attrib["y"] = str(self.y)

class FunctionNode(Node):
    """
    Specification of the Node class for processing functions.  Has a
    C-code constructor with a different form from that of DataNodes.
    """
    def printCConstructor(self, writer):
        # This code assumes that all the output links are connected.  Need to add
        # code to create a virtual object for output if there are no output links.
        writer.printLine("vx%sNode(%s, " % (self.getKernel().fullname, writer.graphName), end='')
        inputs = [port.CCodeName() for port in self.inports]
        outputs = [port.CCodeName() for port in self.outports]
        print("%s, %s);" % (', '.join(inputs), ', '.join(outputs)), file=writer.outfile)

class DataNode(Node):
    """
    Specification of the Node class for data objects.  Has a C-code
    constructor with a different form from that of FunctionNodes.
    """

    # Define the parameter lists and their defaults for the various
    # data nodes.  Needs to be an OrderedDict so that the parameters
    # come out in order in getConstructorParamList() below to generate
    # the "C" code to construct the data objects.
    imageParams = OrderedDict()
    imageParams["width"] = 640
    imageParams["height"] = 480
    imageParams["color"] = "VX_DF_IMAGE_Y800"

    bufferParams = OrderedDict()
    bufferParams["unitsize"] = 4
    bufferParams["numunits"] = 8

    scalarParams = OrderedDict()
    scalarParams["value"] = 1

    defaultParams = dict(IMAGE=imageParams, BUFFER=bufferParams,
                         SCALAR=scalarParams)

    def __init__(self, graph, nodetype, nodeid=None):
        "Add the default attributes for the various data types."
        Node.__init__(self, graph, nodetype, nodeid)
        self.params = DataNode.defaultParams[nodetype].copy()

    def updateAttributesFromElement(self, n):
        "Update the type-specific attributes from the given tree element."
        for param in self.params:
            v = n.get(param)
            if v != None: self.params[param] = str(v)

    def addAttributesToElement(self, elem):
        "Add the type specific attributes to the given tree element."
        Node.addAttributesToElement(self, elem)
        for param, value in self.params.items():
            elem.attrib[param] = str(value)

    def printCConstructor(self, writer):
        "Print the C constructor for the appropriate data object type."
        if len(self.inports[0].links) > 0:  port = self.inports[0]
        if len(self.outports[0].links) > 0: port = self.outports[0]
        writer.printLine("%s %s = vxCreate%s(%s%s);" %
                         (typeTable[port.datatype], port.CCodeName(),
                          typeName(port.datatype), writer.context,
                          self.getConstructorParamList()))

    def getConstructorParamList(self):
        "Generate the constructor's comma-separated parameter list"
        return "".join([", %s" % v for v in self.params.values()])

class Port:
    """
    Port class is the object via which nodes can be linked to one
    another.  The port knows its direction (in or out) and datatype to
    facilitate validating links.  It also keeps a list of links, which
    for input ports can only have one element, but for output links
    can be arbitrarily long.
    """
    def __init__(self, direction, datatype, node):
        self.objecttype = "port"
        self.direction = direction # can be 'in' or 'out'
        self.datatype = datatype
        self.node = node
        self.links = []
        node.addPort(self)

    def addLink(self, link):
        "Adds the given link to the link list (if legal)."
        if self.direction == 'in' and len(self.links) != 0:
            self.graph.setError("Error: only one allowed link per input")
            return None
        self.links.append(link)
        return self

    def orderLink(self, other):
        """Utility function that takes another port, determines which
        one is "in" ('self' or the other one) and which is "out" and
        returns them in (srcPort, dstPort) order, i.e., "out" port
        followed by "in" port.  Assumes the directions are different."""
        if self.direction == "in":
            srcPort = other
            dstPort = self
        else:
            srcPort = self
            dstPort = other
        return (srcPort, dstPort)

    def linkable(self, port2):
        """Check to see if two ports are "linkable" based on various
        criteria.  If they aren't, indicate an error to the graph and
        return False, otherwise return True."""
        port1 = self
        if port1.direction == port2.direction:
            self.graph.setError("Error: directions the same--must link inputs to outputs")
            return False
        if port1.datatype != port2.datatype:
            self.graph.setError("Error: the data types of the two ports don't match.")
            return False
        (src, dst) = port1.orderLink(port2)
        if len(dst.links) != 0:
            self.graph.setError("Error: only one allowed link per input")
            return False
        if dst.node.canGetTo(src.node):
            self.graph.setError("Error: creating this link would cause a cycle")
            return False
        return True

    def CCodeName(self):
        """Return the C-code name of the port object.  If this is an
        input port, follow the link back to its source to get the
        name.  If the containing graph is valid, this will work
        because all inputs most be linked.  Otherwise, if this is a
        DataNode port, then the name is just the name of the node.
        This is a FunctionNode port, need to tack on the output name."""
        if self.direction == 'in':
            return self.links[0].src.CCodeName()
        ccname = "%s%s" % (self.node.nodetype.lower(), self.id[6:-2])
        if isinstance(self.node, FunctionNode):
            outid = self.node.getKernel().outputlist[self.index][0].capitalize()
        else: outid = ''
        return "%s%s" % (ccname, outid)

class Link:
    """
    The Link object is just a src-dst pair with an ID.  Has some
    helper functions to aid in writing the associated C-code.
    """
    def __init__(self, srcPort, dstPort):
        "Construct an ID, initialize src & dst, and add self to graph."
        self.objecttype = "link"
        self.id = "link-%s-to-%s" % (srcPort.id[5:], dstPort.id[5:])
        self.src = srcPort.addLink(self)
        self.dst = dstPort.addLink(self)
        srcPort.node.graph.addObject(self)

    def printCConstructor(self, writer):
        """Print the constructor for a virtual object associated with
        the link, *but only if it needs it*.  If the src or dst is a
        DataNode, then it's not needed.  Also make sure the link is
        the *first* one from the port so we don't construct it more
        than once."""
        if isinstance(self.src.node, DataNode): return
        if isinstance(self.dst.node, DataNode): return
        if self.src.links[0] != self: return
        writer.printLine("%s %s = vxCreateVirtual%s(%s);" %
                         (typeTable[self.src.datatype], self.src.CCodeName(),
                          typeName(self.src.datatype), writer.context))

class Graph:
    """
    The Graph object manages the nodes, ports, and links in
    dictionaries to enable retrieval of objects via their IDs.  Has
    has one "objects" dictionary that points to the other three.
    Doles out node IDs when requested by calling the first node
    requested "node0" and incrementing the index for subsequent nodes.
    Has an error string that can be set by graph object requests and
    retrieved by the user.  Member functions include graph validators,
    data accessors, and information query functions.  Can be
    initialized from scratch (for interactive construction) or from an
    XML file.  Can also be saved to an XML file.
    """

    def __init__(self):
        "Initialize the elements of the graph structure"
        self.nodes = dict()
        self.links = dict()
        self.ports = dict()
        self.objects = dict()
        self.objects["node"] = self.nodes
        self.objects["port"] = self.ports
        self.objects["link"] = self.links
        self.nextnodeid = 0
        self.error = ''

    @classmethod
    def fromTree(cls, tree, nodeClass=Node):
        """Initialize from an elementTree structure.  Create all the
        nodes found in the tree using the attributes indicated,
        together with their corresponding ports.  Assumes the ports of
        a given node are in the correct order in the XML file.
        Finally read the links, get their endpoints from the Port list
        we just created and create the links themselves.
        """
        graph = cls()
        for n in tree.iter("node"):  # Create each node
            node = nodeClass.fromTreeElement(graph, n)
            for p in n.iter("port"):  # Create each port
                port = Port(p.get('direction'), p.get('type'), node)
        for l in tree.iter("link"):  # Create each link
            link = Link(graph.getObject(l.get('src')),
                        graph.getObject(l.get('dst')))
        return graph

    def getNextNodeID(self):
        """Increment the next node ID until the ID generated is new,
        and return the new node ID number.  Can't just use the number
        of nodes in the graph, because some node IDs could have been
        read in from an XML file with arbitrary ID numbers."""
        while ("node"+str(self.nextnodeid) in self.nodes):
            self.nextnodeid = self.nextnodeid + 1
        self.nextnodeid = self.nextnodeid + 1
        return self.nextnodeid - 1

    def allNodesHaveKernels(self):
        """Check to make sure that all the nodes have kernels."""
        for nodeid, node in self.nodes.items():
            if node.kernel == None:
                self.setError("Node %s has no kernel defined." % nodeid)
                return False
        return True

    def allInputsAndOutputsMet(self):
        """Check the graph to make sure that all the nodes that
        represent kernel functions (and not data objects) have inputs."""
        for nodeid, node in self.nodes.items():
            if isinstance(node, FunctionNode):
                for i in range(len(node.inports)):
                    if len(node.inports[i].links) == 0:
                        self.setError("%s %s input %s not met." %
                                      (node.kernel.name, nodeid, i))
                        return False
                for i in range(len(node.outports)):
                    if len(node.outports[i].links) == 0:
                        self.setError("%s %s output %s not met." %
                                      (node.kernel.name, nodeid, i))
                        return False
        return True

    def saveToXML(self, outfile):
        """Save an XML representation of the graph to the given
        outfile.  The format of the XML tree is that it has a "graph"
        at the root, with "nodes" and "links" as children.  The
        "nodes" item is a list of nodes, each of which in turn has a
        list of ports as children.  Nodes, ports, and links have
        appropriate attributes needed to re-create the graph when
        loaded.  The graph assumed to be validated prior to calling.
        """
        docroot = etree.Element("openvx")
        graphroot = etree.SubElement(docroot, "graph")
        nodes = etree.SubElement(graphroot, "nodes")
        links = etree.SubElement(graphroot, "links")
        for node in self.nodes.values():
            n = etree.SubElement(nodes, "node")
            node.addAttributesToElement(n)
            ports = etree.SubElement(n, "ports")
            for port in node.inports:
                p = etree.SubElement(ports, "port")
                p.attrib["id"] = port.id
                p.attrib["type"] = port.datatype
                p.attrib["direction"] = port.direction
            for port in node.outports:
                p = etree.SubElement(ports, "port")
                p.attrib["id"] = port.id
                p.attrib["type"] = port.datatype
                p.attrib["direction"] = port.direction
        for link in self.links.values():
            l = etree.SubElement(links, "link")
            l.attrib["id"] = link.id
            l.attrib["src"] = link.src.id
            l.attrib["dst"] = link.dst.id
        etree.ElementTree(graphroot).write(outfile)

    def addObject(self, obj):
        """Add the given object to the graph.  If it's a new node,
        generate its ID, otherwise the ID must have been generated in
        the object's constructor."""
        obj.graph = self
        if obj.objecttype == "node":
            if obj.id == None:
                obj.id = "node%s" % self.getNextNodeID()
        self.objects[obj.objecttype][obj.id] = obj

    def getObject(self, objID):
        "Retrieve an object via its ID."
        if objID[:4] not in ["node", "port", "link"]: return None
        return self.objects[objID[:4]][objID]

    def deleteObject(self, obj):
        "Remove an object from the graph."
        del self.objects[obj.objecttype][obj.id]

    def numNodes(self):
        "Simple accessor returns the number of nodes in the graph."
        return len(self.nodes)

    def clear(self):
        "Destroy all the objects in the graph and reset it."
        nodelist = self.nodes.copy()
        for node in nodelist: self.nodes[node].delete(None)
        self.nextnodeid = 0
        self.error = ''

    def setError(self, error):
        "Set the 'last error' string."
        self.error = error

    def getLastError(self):
        "Get the 'last error' string."
        return self.error
