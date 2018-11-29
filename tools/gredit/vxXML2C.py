
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

## \file vxXML2C.py
## \author Frank Brill <brill@ti.com>


# Get the I/O file names from the command line.  Make sure they're
# provided before doing anything else.
import sys
if len(sys.argv) > 2:
    XMLfile = sys.argv[1]
    Cfile = sys.argv[2]
else:
    print ("Usage: python %s <infile.xml> <outfile.c>" % sys.argv[0],
           file=sys.stderr)
    exit()

# Load an XML representation of the graph from the given file.
# Parse the file to create a Python ElementTree.
from xml.etree import ElementTree as etree
try:
    tree = etree.parse(XMLfile)
except:
    print("Graph XML Load", "Cannot load file "+filename, file=sys.stderr)
    exit()

# Convert the elementTree into a vxGraph
from vxGraph import Graph
graph = Graph.fromTree(tree)

# Write the resulting graph to the given output file
from vxCodeWriter import GraphCodeWriter
writer = GraphCodeWriter("context", graph)
writer.writeCfile(Cfile)
