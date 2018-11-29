
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


## \file gredit.py
## \author Frank Brill <brill@ti.com>

"""
Implements a GUI for a graph object editor.  The top-level object is
the GrEditor, which has all the GUI elements for menus, buttons, and
most importantly a GraphCanvas, which is the drawing area for the
graph.  The graph consists of nodes with ports, and links between the
ports.  The user can interactively create a graph object and save and
load it from an XML file.  The graph is validated incrementally as it
is constructed, with a final check before saving.

Classes defined are:
    GrEditor: The top-level GUI
    GraphCanvas: The graph drawing area of the GUI
    CanvasObject: An object that can be placed on a GraphCanvas
    CanvasNode, CanvasPort, and CanvasLink: Types (subclasses) of
        CanvasObject and the corresponding 'regular' graph objects
"""

import math
from tkinter import *
import tkinter.font
from tkinter import messagebox
from vxGraph import *
from vxCodeWriter import *

# OPEN ISSUES

# Need to save, restore, and use the data node "name"s in generating code

# Need to clean up how the object dictionaries are maintained and the way icons are mapped
#      to objects.  Could be a bit simpler.  General cleanup needed.  Eliminate globals.
#
# The automatic layout/rearrangement and tree dragging interaction could be improved.
# Would like to be able to select an entire group of nodes and drag them all at once.
# Editing should be more "modal": normally in "move" mode, only add items in "add" mode
# When using the drag method of drawing a link, the target node isn't highlighted (WHY NOT?)
# Should add scroll bars to canvas when the graph gets too big to fit on it.
#
# Should eliminate duplicate code in GraphCanvas.loadFromTree() vs. Graph.fromTree.
# Error checking is sparse (e.g. don't check for quit without saving, etc.)
#
# Should put image, scalar, buffer on a different palette from the kernel nodes?
# Will need better way to select kernels when the kernel list gets large.  Will need
#      scrollbar at least, probably a hierarchy of kernels to navigate.

# Table defining various color constants
colorTable = [('I', "#4444CF"),   # images and unselected image links are blue
              ('B', "#AFAF00"),   # buffers and unselected buffer links are yellow
              ('S', "#00BF00")    # scalars and unselected scalar are green
              ]
kernelNodeColor = "#CF2222"       # kernel nodes are red
defaultOutlineColor = "gray20"    # all node outlines are dark gray
linkHilightColor = "gray80"       # links turn light gray when they're selected
portOutlineColor = "black"        # ports are outlined in black
canvasBackgroundColor = "gray50"  # the background color of the graph canvas
listboxBackgroundColor = "gray80" # the background color of the kernel list

class CanvasObject:
    """
    A CanvasObject is something that can be placed on a GraphCanvas.
    It is the superclass for CanvasNode, CanvasPort, and CanvasLink.
    """

    def __init__(self, canvas, x, y, w, h):
        """Store the object's size (w, h), location (x, y), and the
        canvas it lives on."""
        self.canvas = canvas
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    def attachIcon(self, icon, objecttype):
        """Store the subclass type string (node, port, or link) and
        the actual Tk canvas icon representing the object, bind some
        GUI interactions to it, and update the canvas' object
        dictionary.  Should only be called by the subclass
        constructors."""
        self.icon = icon
        self.canvas.addtag_withtag(self.id, self.icon)
        self.canvas.tag_bind(self.id, "<Any-Enter>", self.highlight)
        self.canvas.tag_bind(self.id, "<Any-Leave>", self.unhighlight)
        self.canvas.tag_bind(self.id, "<Button-3>", self.delete)

    def highlight(self, event):
        """Highlight the object by thickening its outline, and set the
        current focus to this object."""
        self.canvas.focus_set()
        self.canvas.currentObject = self
        self.canvas.itemconfig(self.icon, {'width':'3'})

    def unhighlight(self, event):
        """Unhighlight the object, and clear the current focus."""
        self.canvas.currentObject = None
        self.canvas.itemconfig(self.icon, {'width':'1'})

    def move(self, dx, dy):
        """Move the object by an amount relative to the current location."""
        self.x = self.x + dx
        self.y = self.y + dy
        self.canvas.move(self.icon, dx, dy)

    def moveTo(self, x, y):
        """Move the object to an absolute location on the canvas."""
        self.x = x
        self.y = y

    def delete(self, event):
        """Destroy the icon and remove self from the graph."""
        self.canvas.delete(self.icon)
        self.canvas.graph.deleteObject(self)


class CanvasNode(Node, CanvasObject):
    """
    A CanvasNode is a CanvasObject that represents a particular
    kernel, image, buffer, or scalar.  Each of these types of nodes
    has an icon of a characteristic shape and color.  CanvasNodes have
    lists of input and output CanvasPorts, which can be connected by
    CanvasLinks.
    """
    def __init__(self, canvas, x, y, kernel, nodeid=None):
        Node.__init__(self, canvas.graph, kernel.name, nodeid)
        CanvasObject.__init__(self, canvas, x, y, 25, 12)
        self.kernel = kernel

    def finishInit(self, icon):
        """Attach the given icon and label and place the icon at the
        given (x, y), location."""
        self.attachIcon(icon, "node")
        if self.kernel == None: self.nodetype = self.id # if no kernel assigned, just use ID
        else: self.nodetype = self.kernel.name
        self.label = self.canvas.create_text((self.x, self.y), tags=self.id,
                                             text=self.nodetype, fill='white')
        labelfont = self.canvas.itemcget(self.label, 'font')
        myfont = tkinter.font.Font(font=labelfont)
        myfont['weight'] = 'bold'
        self.canvas.itemconfig(self.label, font=myfont)
        self.moveTo(self.x, self.y)

    @classmethod
    def fromTreeElement(cls, canvas, n):
        """Create a node based on the information in the tree element."""
        if n.get('type') in Kernel.dataKernels:
            return CanvasDataNode.fromTreeElement(canvas, n)
        else: return CanvasFunctionNode(canvas, float(n.get('x')), float(n.get('y')),
                                        canvas.app.kernel[n.get('type')], n.get('id'))

    @classmethod
    def fromEventAndKernel(cls, event, kernel):
        """Create a node at the event indicated in the event with the
        given kernel."""
        if kernel == None: return None
        if kernel.name in Kernel.dataKernels:
            return CanvasDataNode(event.widget, event.x, event.y, kernel)
        else: return CanvasFunctionNode(event.widget, event.x, event.y, kernel)

    def createPortsForKernel(self):
        if self.kernel != None:
            for i in range(self.kernel.numInputs):
                CanvasPort(self, "in", self.kernel.inputs[i])
            for i in range(self.kernel.numOutputs):
                CanvasPort(self, "out", self.kernel.outputs[i])
        else:
            CanvasPort(self, "in", 'I')
            CanvasPort(self, "out", 'I')
        self.moveTo(self.x, self.y)

    def move(self, dx, dy):
        """Move the entire node's icon, including shape, label, and
        ports by a relative amount.  The moveTo() method must have
        been called at least once prior to this to correctly place the
        port and label sub-icons."""
        CanvasObject.move(self, dx, dy)
        self.canvas.move(self.label, dx, dy)
        for port in self.inports: port.move(dx, dy)
        for port in self.outports: port.move(dx, dy)
        return (dx, dy)

    def moveTo(self, x, y):
        """Place the node at an absolute (x, y) location on the
        canvas.  Correctly lay out all the sub-parts of the icon,
        including shape, label, and ports."""
        CanvasObject.moveTo(self, x, y)
        self.canvas.coords(self.icon, x-self.w, y-self.h, x+self.w, y+self.h)
        self.canvas.coords(self.label, x, y)
        for i in range(len(self.inports)):
            dx = 2*self.w*(i+1)/(len(self.inports)+1) - self.w
            dy = self.h*math.sqrt(1.0 - (dx*dx)/(self.w*self.w))
            self.inports[i].moveTo(x+dx, y-dy)
        for i in range(len(self.outports)):
            dx = 2*self.w*(i+1)/(len(self.outports)+1) - self.w
            dy = self.h*math.sqrt(1.0 - (dx*dx)/(self.w*self.w))
            self.outports[i].moveTo(x+dx, y+dy)

    def moveTreeTo(self, x, y):
        """Move the node to an absolute location and drag the subtree."""
        dx = x - self.x
        dy = y - self.y
        self.moveTree(dx, dy);

    def moveTree(self, dx, dy):
        """Wrapper around recursive function to the node and its
        entire subtree by a relative amount."""
        movedNodes = set() # Keep track of the nodes we've moved
        self.moveTreeRecursive(dx, dy, movedNodes)

    def moveTreeRecursive(self, dx, dy, movedNodes):
        """Recursive function to move the node and its entire subtree
        by a relative amount."""
        self.move(dx, dy)
        movedNodes.add(self.id) # Mark node as moved so recursion
        for port in self.outports:  # doesn't move it more than once
            for link in port.links:
                if not link.dst.node.id in movedNodes:
                    link.dst.node.moveTreeRecursive(dx, dy, movedNodes)

    def delete(self, event):
        """Destroy the node, its label, and all of its ports."""
        self.canvas.delete(self.label)
        for port in self.inports: port.delete(event)
        for port in self.outports: port.delete(event)
        CanvasObject.delete(self, event)

class CanvasDataNode(CanvasNode, DataNode):
    """
    A canvas node that represents a data object instead of a function.
    They look and behave somewhat differently from function nodes, so
    we have two subclasses.
    """
    def __init__(self, canvas, x, y, kernel, nodeid=None):
        CanvasNode.__init__(self, canvas, x, y, kernel, nodeid)
        DataNode.__init__(self, canvas.graph, kernel.name, nodeid)

        fill = self.canvas.dataColor[self.kernel.name[0]]
        self.w = self.w - 5
        self.h = self.h - 5
        icon = self.canvas.create_rectangle(-self.w, -self.h, self.w, self.h,
                                            fill=fill, width=1, outline=defaultOutlineColor)
        self.finishInit(icon)

    @classmethod
    def fromTreeElement(cls, canvas, n):
        """Create a data node based on the information in the tree element."""
        node = CanvasDataNode(canvas, float(n.get('x')), float(n.get('y')),
                              canvas.app.kernel[n.get('type')], n.get('id'))
        node.updateAttributesFromElement(n)
        return node

class CanvasFunctionNode(CanvasNode, FunctionNode):
    """
    A canvas node that represents a function object.  They look and
    behave somewhat differently from data nodes, so we have two
    subclasses.
    """
    def __init__(self, canvas, x, y, kernel, nodeid=None):
        CanvasNode.__init__(self, canvas, x, y, kernel, nodeid)
        icon = self.canvas.create_oval(-self.w, -self.h, self.w, self.h,
                                       fill=self.canvas.nodeColor, width=1,
                                       outline=defaultOutlineColor)
        self.finishInit(icon)

class CanvasPort(Port, CanvasObject):
    """
    A CanvasPort is a CanvasObject that represents an input or output
    parameter of a CanvasNode.  A port has a direction ("in" or "out")
    and a data type (image, buffer, or scalar).  These attributes need
    to be checked when linking ports together.
    """

    def __init__(self, node, direction, datatype):
        """Store the CanvasPort's node, direction, type, and links
        (initially the empty set).  Creates the icon and binds some
        GUI interaction to it."""
        Port.__init__(self, direction, datatype, node)
        CanvasObject.__init__(self, node.canvas, 0, 0, 2, 2)
        color = node.canvas.dataColor[datatype]
        polygon = self.canvas.create_polygon(-self.w, -self.h, self.w, -self.h,
                                             self.w, self.h, -self.w, self.h,
                                             fill=color, outline=portOutlineColor)
        self.attachIcon(polygon, "port")
        self.canvas.tag_bind(self.id, "<Button-3>", self.node.delete)
        self.canvas.tag_bind(self.id, '<B1-Enter>', self.highlight) # doesn't help (WHY NOT?)
        node.moveTo(node.x, node.y)

    def move(self, dx, dy):
        """Move the CanvasPort by an amount relative to the current location
        and drag the links along for the ride."""
        CanvasObject.move(self, dx, dy)
        for link in self.links: link.moveTo(0, 0)

    def moveTo(self, x, y):
        """Move the CanvasPort relative to to an absolute location on the
        canvas and drag the links along for the ride."""
        CanvasObject.moveTo(self, x, y)
        self.canvas.coords(self.icon, (x-self.w, y-self.h, x+self.w, y-self.h,
                                       x+self.w, y+self.h, x-self.w, y+self.h))
        for link in self.links: link.moveTo(0, 0)

    def delete(self, event):
        """Delete the CanvasPort *and* all the links that go to or from it."""
        linklist = list(self.links)
        for link in linklist: link.delete(event)
        CanvasObject.delete(self, event)

class CanvasLink(Link, CanvasObject):
    """
    A CanvasLink is a CanvasObject that represents a connection from an
    output port of one node to the input port of another one.  It is
    typed (image, buffer, or scalar) and this type needs to match on
    both ends of the link.
    """

    def __init__(self, port1, port2):
        """Store the CanvasLink's endpoints and data type, and create the
        line object that graphically represents the link."""
        (src, dst) = port1.orderLink(port2)
        Link.__init__(self, src, dst)
        CanvasObject.__init__(self, self.src.canvas, 0, 0, 0, 0)
        fill = self.canvas.dataColor[self.src.datatype]
        line = self.canvas.create_line(self.src.x, self.src.y,
                                       self.dst.x, self.dst.y, width=3,
                                       fill=fill)
        self.canvas.tag_lower(line)
        self.attachIcon(line, "link")
        self.moveTo(self.x, self.y)

    def highlight(self, event):
        """Highlight the link by changing its color and grab the focus."""
        self.canvas.focus_set()
        self.canvas.currentObject = self
        self.canvas.itemconfig(self.icon, fill=linkHilightColor)

    def unhighlight(self, event):
        """Un-highlight the link to make it specific to data type and
        release the focus."""
        self.canvas.currentObject = None
        fill = self.canvas.dataColor[self.src.datatype]
        self.canvas.itemconfig(self.icon, fill=fill)

    def moveTo(self, x, y):
        """Move line to match the endpoints.  The (x, y) arguments are ignored."""
        CanvasObject.moveTo(self, x, y)
        self.canvas.coords(self.icon, self.src.x, self.src.y,
                           self.dst.x, self.dst.y)

    def delete(self, event):
        """Remove link from endpoints and delete the line."""
        self.src.links.remove(self)
        self.dst.links.remove(self)
        CanvasObject.delete(self, event)


class GraphCanvas(Canvas):
    """
    A GraphCanvas is Tk canvas object with special facilities to
    enable the user to create, save, and load graph objects consisting
    of nodes and links.
    """

    def __init__(self, root, app):
        """Initialize the canvas and data structures needed to
        maintain the graph: the set of nodes, ports, and links
        (maintained in Python dictionaries).  Set the editing mode to
        "normal".
        """

        Canvas.__init__(self, root, width=400, height=300)

        self.root = root
        self.app = app
        self.graph = Graph()
        self.currentObject = None
        self.currNode = None
        self.setMode("normal")

        global colorTable, kernelNodeColor
        self.dataColor = dict()
        for c in colorTable: self.dataColor[c[0]] = c[1]
        self["bg"] = canvasBackgroundColor
        self.nodeColor = kernelNodeColor

    def clear(self):
        """Delete all the nodes, which in turn deletes the ports and links.
        Confirm that the user wants to do this, and if not, just return False.
        Otherwise, go ahead and clear the canvas and return True.
        """
        if self.graph.numNodes() == 0: return True
        if not messagebox.askokcancel("Graph Clear",
                                      "Are you sure you want to clear the graph canvas?"):
                return False
        self.graph.clear()
        self.currentObject = None
        self.currNode = None
        self.setMode("normal")
        self.app.message("Graph cleared.")
        return True

    def setMode(self, mode):
        """Set the current editing mode.  Currently support a "normal"
        mode for drawing and moving nodes, and a "drawlink" mode for
        creating link between ports on nodes."""
        if mode == "normal":
            self.bind('<Button-1>', self.makeOrDragNodeOrStartLink)
            self.bind('<Double-Button-1>', self.editCurrentObject)
            self.bind('<B1-Motion>', self.dragCurrNode)
            self.bind('<ButtonRelease-1>', self.autoArrange)
            self.unbind('<Any-Motion>')
            self.bind('<Delete>', self.deleteCurrentObject)
            self.bind('<Left>', self.nudgeCurrentObject)
            self.bind('<Right>', self.nudgeCurrentObject)
            self.bind('<Up>', self.nudgeCurrentObject)
            self.bind('<Down>', self.nudgeCurrentObject)
        elif mode == "drawlink":
            self.unbind("<Delete>")
            self.unbind("<Left>")
            self.unbind("<Right>")
            self.unbind("<Up>")
            self.unbind("<Down>")
            self.bind('<Button-1>', self.completeLink)
            self.unbind('<B1-Motion>')
            self.bind('<ButtonRelease-1>', self.completeLink)
            self.bind('<Any-Motion>', self.moveTempLink)

    def getTopItemAtLocation(self, x, y):
        """Return the item at the given location on the canvas."""
        itemsHere = self.find_overlapping(x, y, x, y)
        if (len(itemsHere) > 0): return itemsHere[-1]
        else: return None

    def getObjectFromIcon(self, item):
        """Given a canvas icon, return the object structure it
        represents."""
        tags = self.gettags(item)
        for tag in tags: # look up all the tags until we get a hit
            obj = self.graph.getObject(tag)
            if obj != None: return obj
        return None

    def makeOrDragNodeOrStartLink(self, event):
        """Initiate the user-directed action when the user has a mouse
        interaction with the canvas in "normal" mode.  If the user
        clicks on an empty spot on the canvas, create a node at the
        click location.  If they click on a node, get ready to
        drag/move it.  If they click on a port, get ready to draw a
        link."""
        thisItem = self.getTopItemAtLocation(event.x, event.y)
        if thisItem == None:  # Nothing at selected location, so create a new node
            kernel = self.app.getCurrentKernel()
            self.currNode = CanvasNode.fromEventAndKernel(event, kernel)
            if self.currNode != None: self.currNode.createPortsForKernel()
        else:
            obj = self.getObjectFromIcon(thisItem)
            if (obj.objecttype == "node"): # Start dragging selected node
                self.currNode = obj
            elif (obj.objecttype == "port"): # Start creating link from selected port
                self.currNode = obj.node
                self.newLinkSrc = obj
                self.tempLink = self.create_line(obj.x, obj.y, event.x, event.y,
                                                 tag="templine")
                self.tag_lower(self.tempLink)
                self.setMode("drawlink")

    def moveTempLink(self, event):
        """Update the "rubber-banding" line (tempLink) that represents
        the new link being created."""
        self.coords(self.tempLink, self.newLinkSrc.x, self.newLinkSrc.y,
                    event.x, event.y)

    def deleteCurrentObject(self, event):
        """Delete the object at the mouse event location.  If it's a
        port, delete the corresponding node too."""
        if self.currentObject != None:
            if self.currentObject.objecttype == "port":
                self.currentObject.node.delete(event)
            else: self.currentObject.delete(event)

    def editCurrentObject(self, event):
        """Pop up an attribute editor for the current object."""
        # Some ugly stuff in here--needs cleanup.  This pop-up thing
        # works, but is clunky.  Should be able to just edit the label
        # text directly.  Check out the Quad "display" method in
        # quadcanvas.py to see how this is done.
        if not isinstance(self.currentObject, DataNode): return # only edit data nodes
        editObj = self.currentObject # Need to save this because 
        self.app.editDataNode(editObj, event) # editing may reset currentObject
        if editObj.name != '':
            self.itemconfig(editObj.label, text=editObj.name)
        else: self.itemconfig(editObj.label, text=editObj.kernel.name)

    def nudgeCurrentObject(self, event):
        """Move the current object one pixel in direction indicated by
        an arrow keypress event."""
        if self.currentObject != None:
            if self.currentObject.objecttype == "node":
                if event.keysym == "Up":
                    self.currentObject.move(0, -1)
                elif event.keysym == "Down":
                    self.currentObject.move(0, 1)
                elif event.keysym == "Left":
                    self.currentObject.move(-1, 0)
                elif event.keysym == "Right":
                    self.currentObject.move(1, 0)

    def completeLink(self, event):
        """Check to make sure that the user has indicated a valid link
        between compatible ports, and if so, create the link and drop
        back into "normal" mode."""
        dstIcon = self.getTopItemAtLocation(event.x, event.y)
        if dstIcon != None:
            dst = self.getObjectFromIcon(dstIcon)
            if dst == self.newLinkSrc: return
            self.delete(self.tempLink)
            if (dst != None) and (dst.objecttype == "port"):
                if dst.linkable(self.newLinkSrc):
                    newLink = CanvasLink(self.newLinkSrc, dst)
                    self.autoArrange(event)
                else: self.app.message(self.graph.getLastError())
        self.setMode("normal")

    def dragCurrNode(self, event):
        """Move the current node to the location indicated by the mouse event."""
        if self.currNode != None:
            self.currNode.moveTreeTo(event.x, event.y)

    def saveAll(self):
        """Save an XML representation of the graph to the app's
        current saveFile.  The graph is validated (checked to make
        sure all of its inputs and outputs are met and all nodes have
        kernels attached) before saving.
        """
        if not self.graph.allNodesHaveKernels():
            self.app.message(self.graph.getLastError())
            return
        if not self.graph.allInputsAndOutputsMet():
            self.app.message(self.graph.getLastError())
            return
        filename = self.app.getSaveFile()
        if os.path.exists(filename):
            if not messagebox.askokcancel("Graph XML Save",
                                          "File %s exists.  Overwrite?" %
                                          filename): return
        self.graph.saveToXML(filename)
        self.app.message("Saved", filename)

        # Also write the corresponding C file
        filename = os.path.splitext(filename)[0]+'.c'
        writer = GraphCodeWriter("context", self.graph)
        writer.writeCfile(filename)

    def loadFile(self):
        """Load an XML representation of the graph from the app's
        current saveFile.  Parse the file to create a Python
        ElementTree.  Make sure the user is OK with clearing the
        graph, and if so, initialize graph from the tree and notify
        the user that we're done.
        """
        try:
            tree = etree.parse(self.app.getSaveFile())
        except:
            messagebox.showerror("Graph XML Load",
                                 "Cannot load file "+self.app.getSaveFile())
            return
        if self.clear():
            self.loadFromTree(tree)
            self.app.message("Loaded", self.app.getSaveFile())

    def loadFromTree(self, tree):
        """Create a graph structure from an elementTree structure.
        This function mirrors the Graph.fromTree function exactly.
        Still trying to figure out how to make that one create Canvas
        objects so we can do away with this function.
        """
        for n in tree.iter("node"):  # Create each node
            node = CanvasNode.fromTreeElement(self, n)
            for p in n.iter("port"):  # Create each port
                port = CanvasPort(node, p.get('direction'), p.get('type'))
        for l in tree.iter("link"):  # Create each link
            CanvasLink(self.graph.getObject(l.get('src')),
                       self.graph.getObject(l.get('dst')))

    def autoArrange(self, event):
        """Arrange the nodes to make the graph lay out prettier."""
        self.enforceTopToBottom(event)
        self.spreadNodes(event)
        return

    def enforceTopToBottom(self, event):
        """Make sure children are below their parents in the graph."""
        speed = 0.5
        while True:
            changed = False
            for link in self.graph.links.values():
                if link.src.y > link.dst.y - 10:
                    link.src.node.move(0, -speed)
                    link.dst.node.moveTree(0, speed)
                    self.root.update_idletasks()
                    changed = True
            if not changed: break
        return

    def sign(self, x):
        """Utility to determine the sign of a number."""
        if x < 0: return -1;
        if x > 0: return 1;
        return 0;

    def spreadNodes(self, event):
        """Iteratively move the node icons to auto-arrange them such
        that nodes aren't on top of each other."""
        speed = 0.5
        while True:
            changed = False
            for thisNode in self.graph.nodes.values():
                (fx, fy) = (0, 0)
                for otherNode in self.graph.nodes.values():
                    if thisNode != otherNode:
                        (dx, dy) = (thisNode.x-otherNode.x,
                                    thisNode.y-otherNode.y)
                        if ((abs(dx) < thisNode.w+otherNode.w) and
                            (abs(dy) < thisNode.h+otherNode.h)):
                            (fx, fy) = (fx + self.sign(dx), fy + self.sign(dy))
                            if (dx, dy) == (0, 0): (fx, fy) = (1, 1)
                            thisNode.move(fx*speed, fy*speed)
                            self.root.update_idletasks()
                            changed = True
            if not changed: break

class GrEditor:

    def __init__(self):
        """Initialize the table of kernels and create all the GUI
        widgets, including the GraphCanvas, button controls, message
        area, load/store filename text widget, and kernel selection
        list, and then launch the app.
        """

        self.kernel = Node.kernelTable

        root = Tk()
        root.title("Graph Editor")
        self.defaultSaveFile = "mygraph.xml"
        self.root = root

        controls = Frame(root)
        self.messageString = StringVar()
        msg = Message(controls, anchor=W, width=330, textvariable=self.messageString)
        canv = GraphCanvas(root, self)

        self.listbox = Listbox(root, width=9, bg=listboxBackgroundColor)
        for k in self.kernel.keys(): self.listbox.insert(END, k)
        self.listbox.select_set(0)

        load = Button(controls, text="Load", command=canv.loadFile)
        save = Button(controls, text="Save", command=canv.saveAll)
        clear = Button(controls, text="Clear", command=canv.clear)
        quit = Button(controls, text="Quit", command=root.quit)
        msg.pack({"side":"left", "fill":X, "expand":1})
        load.pack({"side":"left"})
        save.pack({"side":"left"})
        clear.pack({"side":"left"})
        quit.pack({"side":"left"})

        self.fileEntry = Entry(root)
        self.fileEntry.insert(0, self.defaultSaveFile)

        controls.pack(side="bottom", fill=X)
        self.listbox.pack(side="left", fill=Y)
        self.fileEntry.pack(side="top", fill=X)
        canv.pack(fill=BOTH, expand=1)
        canv.focus_set()
        self.messageString.set("Hello")
        root.mainloop()
        exit()

    def addField(self, window, form, name, value):
        "Add a field to a form in the given window"
        paramArea = Frame(window)
        Label(paramArea, text=name).pack(side='left')
        form[name] = Entry(paramArea)
        form[name].pack(padx=5, side='right', fill=X)
        form[name].insert(0, value)
        paramArea.pack()

    def editDataNode(self, node, event):
        "Create a popup to edit a data node's attributes."
        self.nodeForm = dict()
        self.nodeBeingEdited = node
        self.editPopup = Toplevel(self.root)
        self.addField(self.editPopup, self.nodeForm, 'name', node.name)
        for param, value in node.params.items():
            self.addField(self.editPopup, self.nodeForm, param, value)
        button = Button(self.editPopup, text="OK", command=self.ok)
        button.pack(pady=5)
        self.editPopup.grab_set()
        self.editPopup.geometry("+%s+%s" % (event.x+self.root.winfo_rootx(),
                                            event.y+self.root.winfo_rooty()))
        self.root.wait_window(self.editPopup)
        return

    def ok(self):
        "Stuff the form values into the node being edited"
        self.nodeBeingEdited.name = self.nodeForm['name'].get()
        for param in self.nodeBeingEdited.params:
            self.nodeBeingEdited.params[param] = self.nodeForm[param].get()
            # should use default if blank
        self.editPopup.destroy()

    def getSaveFile(self):
        "Return the name of the user-supplied filename."
        return self.fileEntry.get()

    def message(self, *args):
        """Display the given message in the app's message area.
        Format the arguments similar to the print() function."""
        self.messageString.set(''.join([str(arg)+" " for arg in args])[:-1])

    def getCurrentKernel(self):
        """Return the kernel the user has indicated in the selection box."""
        selection = self.listbox.curselection()
        if len(selection) == 0: return None
        elif len(selection) == 1:
            return self.kernel[self.listbox.get(selection[0])]
        else:
            print("ERROR: multiple selections")
        return None

app = GrEditor()
