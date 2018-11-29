
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

## \file vxWriteNodeHeaders.py
## \author Frank Brill <brill@ti.com>

"""
Simple script that pulls in the node table defined in vxNodes.py, and
uses the code in vxNodeUtils.py to check that it's clean, and write
out a C header file for the nodes defined in the node table.  The name
of the output file is provided on the command line.
"""

import sys

nodeFilename = None
kernelFilename = None

# Get the output file name
if len(sys.argv) > 2:
    nodeFilename = sys.argv[1]
    kernelFilename = sys.argv[2]
else:
    print ("Usage: python %s <vx_nodes.h> <vx_kernels.h>" % sys.argv[0], file=sys.stderr)
    exit()

# Pull in the node table and utility functions
from vxNodes import vxNodeTable
from vxNodeUtils import checkNodeTable, writeNodeHeaderFile, writeKernelHeaderFile

# Try to write the file and inform the user of success or failure
if checkNodeTable(vxNodeTable):
    try: # write the node header file
        writeNodeHeaderFile(vxNodeTable, nodeFilename)
    except:
        print("Error writing file %s...exiting." % nodeFilename, file=sys.stderr)
        raise

    print("Node header file successfully written to", nodeFilename, file=sys.stdout)

    try: # write the kernel header file
        writeKernelHeaderFile(vxNodeTable, kernelFilename)
    except:
        print("Error writing file %s...exiting." % kernelFilename, file=sys.stderr)
        raise

    print("Kernel header file successfully written to", kernelFilename, file=sys.stdout)
