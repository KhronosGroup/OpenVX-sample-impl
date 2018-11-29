
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

## \file vxKernel.py
## \author Frank Brill <brill@ti.com>

"""
Implements the Kernel class for VX functions.  It's really just a
structure that holds the name of the kernel and the number and types
of its inputs and outputs.  The inputs and outputs are specifed via a
character string, where "I" means image, "B" means buffer, and "S"
means scalar.

Classes defined are:
    Kernel: Describes a kernel function interface: its name and the
            number and types of its I/O's
"""

from vxTypes import *
from vxNodes import vxNodeTable
from collections import OrderedDict

class Kernel:
    """The Kernel class is really just a structure that holds the name
    of the kernel and the number and types of its inputs and outputs.
    The inputs and outputs are specifed via a character string, where
    "I" means image, "B" means buffer, and "S" means scalar.
    """

    # The table defining the kernels supported
    kernelTable = OrderedDict()
    dataKernels = ("IMAGE", "BUFFER", "SCALAR")

    def __init__(self, name, inputs, outputs):
        """Initialize the kernel structure members.  The inputs and
        outputs are represented as character strings.  For example,
        the constructor call Kernel("FOO", "SI", "B") means that the
        "FOO" kernal has two inputs, the first being a scalar and the
        second being an image, and it has one output buffer.
        """
        self.name = name
        self.inputs = inputs.upper()
        self.outputs = outputs.upper()
        self.numInputs = len(inputs)
        self.numOutputs = len(outputs)

    @classmethod
    def fromNodeTableEntry(cls, node):
        """Initialize the kernel structure members from a node table
        entry.  Capture all the data there."""

        nodeinfo, inputlist, outputlist = node
        name, kernel, shortname, desc = nodeinfo
        inputspec = ''.join(map(Kernel.typechar, inputlist))
        outputspec = ''.join(map(Kernel.typechar, outputlist))

        k = cls(shortname, inputspec, outputspec)
        k.fullname = name
        k.kernelImplementation = kernel
        k.description = desc
        k.inputlist = inputlist
        k.outputlist = outputlist
        return k


    @classmethod
    def typechar(cls, param):
        """Returns 'I', 'B', or 'S' depending on whether the parameter
        is an image, buffer, or scalar."""
        pname, ptype, pdesc = param
        
        if ptype == "vx_image": return 'I'
        if ptype == "vx_buffer": return 'B'
        return 'S'

    @classmethod
    def initTable(cls):
        """Make a list of Kernel objects from the serialized
        information in the node table."""

        # Always include the data kernels first
        Kernel.kernelTable["IMAGE"] = Kernel("IMAGE", "I", "I")
        Kernel.kernelTable["BUFFER"] = Kernel("BUFFER", "B", "B")
        Kernel.kernelTable["SCALAR"] = Kernel("SCALAR", "S", "S")

        for node in vxNodeTable:
            k = Kernel.fromNodeTableEntry(node)
            Kernel.kernelTable[k.name] = k

    @classmethod
    def getTable(cls):
        "Accessor for the table"
        return Kernel.kernelTable

# Initialize the kernel table
Kernel.initTable()
