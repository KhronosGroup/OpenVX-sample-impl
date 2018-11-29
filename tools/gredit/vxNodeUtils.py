
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

## \file vxWriteNodeUtils.py
## \author Frank Brill <brill@ti.com>

"""
Contains several functions for validating a node table (typically kept
in vxNodes.py.  The main interface functions are checkNodeTable() to
verify that a node table is properly formed, writeNodeHeaderFile()
which uses a properly formed node table to write the corresponding C
header file, and writeKernelHeaderFile() that writes another header
file for the kernel enums.
"""

import sys

# Constant used in error messages
ordinal = ("First", "Second", "Third", "Fourth", "Fifth", "Sixth",
           "Seventh", "Eighth", "Ninth", "Tenth")

def printNodeInfo(nodeinfo, outfile):
    "Simple utility prints the name and description of a node."
    name, kernel, shortname, desc = nodeinfo
    print("NODE ->\tConstructor: vx%sNode" % name)
    print("\tKernel: VX_KERNEL_%s" % kernel)
    print("\tShort name: %s" % shortname)
    print("\tDESCRIPTION:")
    for descline in desc.splitlines():
        print("\t%s" % descline)

def printNodeTableEntry(node):
    "Prints a full node entry, including the input and output lists."
    nodeinfo, inputlist, outputlist = node
    printNodeInfo(nodeinfo, sys.stdout)
    for inparam in inputlist:
        print("\tInput: %s is a %s -- %s" % inparam)
    for outparam in outputlist:
        print("\tOutput: %s is a %s -- %s" % outparam)

def printCFunctionHeader(node):
    "Print the C function header for one node"
    nodeinfo, inputlist, outputlist = node
    name, kernel, shortname, desc = nodeinfo

    desclines = desc.splitlines()

    # print the comment
    print("/*!")
    print(" * \\brief", desclines[0])
    for line in desclines[1:]:
        print(" *", line)
    print(" * \\param [in] graph The handle to the graph.")
    for param in inputlist:
        pname, ptype, pdesc = param
        print(" * \\param [in]", pname, pdesc)
    for param in outputlist:
        pname, ptype, pdesc = param
        print(" * \\param [out]", pname, pdesc)
    print(" * \\see VX_KERNEL_%s" % kernel)
    print(" */")

    # print the function header
    # the end=<something> syntax was causing this to not get all the way through the python interp.
    print("vx_node vx%sNode(vx_graph graph, " % name, '')

    # print the inputs
    for param in inputlist[:-1]:
        pname, ptype, pdesc = param
        print("%s %s, " % (ptype, pname), '')
    pname, ptype, pdesc = inputlist[-1]
    print("%s %s" % (ptype, pname), '')

    # print the outputs
    if len(inputlist) > 0: print(", ", '')
    for param in outputlist[:-1]:
        pname, ptype, pdesc = param
        print("%s %s, " % (ptype, pname), '')
    pname, ptype, pdesc = outputlist[-1]
    print("%s %s" % (ptype, pname), '')
    print(");\n") # done

def printCKernelEnum(node, writtenKernels):
    """Prints the C enum for a kernel.  It sends it to stdout, but
    this is usually remapped to a specific file by a wrapper function."""
    nodeinfo, inputlist, outputlist = node
    name, kernel, shortname, desc = nodeinfo

    if kernel in writtenKernels: return
    desclines = desc.splitlines()

    # print the comment
    print("    /*!")
    print("     * \\brief", desclines[0])
    for line in desclines[1:]:
        print("     *", line)
    for param in inputlist:
        pname, ptype, pdesc = param
        print("     * \\param [in]", pname, pdesc)
    for param in outputlist:
        pname, ptype, pdesc = param
        print("     * \\param [out]", pname, pdesc)
    print("     */")

    # print the enum
    print("    VX_KERNEL_%s,\n" % kernel)

    writtenKernels.add(kernel)

def writeNodeHeaderFile(vxNodeTable, filename=None):
    """Prints the entire C header file for the nodes defined in the
    given node table.  First tries and open the output file, and if
    successful pull in the standard copyright header and send it and
    some other boilerplate to the given file.  Then loop over the node
    specifiers in the table and print the corresponding constructor
    function header.  Wrap up with some more boilerplate, close the
    file and return."""

    # re-direct the "print" output to the given file--default to stdout
    if filename:
        try:
            outfile = open(filename, "w")
        except:
            print("Failed to open", filename)
            raise
        tmpstdout = sys.stdout
        sys.stdout = outfile

    # print the standard copyright header
    header = open("vx_header.h", "r")
    for line in header: print(line, '')
    header.close()

    # print the file-specific header
    print("""
#ifndef _OPENVX_NODES_H_
#define _OPENVX_NODES_H_

/*!
 * DO NOT EDIT THIS FILE.  IT IS AUTOMATICALLY GENERATED FROM THE
 * TABLE IN vxNodes.py USING vxWriteNodeHeaders.py IN THE tools
 * DIRECTORY.  TO ADD, DELETE, OR CHANGE NODE DEFINITIONS, EDIT THE
 * TABLE IN vxNodes.py.
 */

/*!
 * \\file vx_nodes.h
 * \\brief The "Simple" API interface for OpenVX. These APIs are just
 * wrappers around the more verbose functions defined in \\ref vx_api.h.
 * \\author Erik Rainey <erik.rainey@gmail.com>
 */
""")

    # print all the node constructor headers
    for node in vxNodeTable:
        printCFunctionHeader(node)

    # close the ifdef
    print("#endif")

    # clean up
    if filename:
        sys.stdout = tmpstdout
        outfile.close()


def writeKernelHeaderFile(vxNodeTable, filename=None):
    """Prints the entire C header file for the kernels defined in the
    given node table.  First tries and open the output file, and if
    successful pull in the standard copyright header and send it and
    some other boilerplate to the given file.  Then loop over the node
    specifiers in the table and print the corresponding kernel enum
    function header.  Wrap up with some more boilerplate, close the
    file and return."""

    # re-direct the "print" output to the given file--default to stdout
    if filename:
        try:
            outfile = open(filename, "w")
        except:
            print("Failed to open", filename)
            raise
        tmpstdout = sys.stdout
        sys.stdout = outfile

    # print the standard copyright header
    header = open("vx_header.h", "r")
    for line in header: print( line, '' )
    header.close()

    # print the file-specific header
    print("""
#ifndef _OPEN_VISION_LIBRARY_KERNELS_H_
#define _OPEN_VISION_LIBRARY_KERNELS_H_

/*!
 * DO NOT EDIT THIS FILE.  IT IS AUTOMATICALLY GENERATED FROM THE
 * TABLE IN vxNodes.py USING vxWriteNodeHeaders.py IN THE tools
 * DIRECTORY.  TO ADD, DELETE, OR CHANGE KERNEL DEFINITIONS, EDIT THE
 * TABLE IN vxNodes.py.
 */

/*!
 * \\file
 * \\brief The list of supported kernels in the OpenVX standard.
 * \\author Erik Rainey <erik.rainey@gmail.com>
 * \\todo Add continue to define \\ref vx_kernel_e as "flat API" evolves.
 * \\todo Add string names to each kernel to use with \\ref vxGetParameterByName.
 */

#ifdef  __cplusplus
extern "C" {
#endif

/*!
 * \\brief The standard list of available vision kernels.
 *
 * Each kernel listed here can be used with the \\ref vxGetKernelByEnum call.
 * When programming the parameters, use
 * \\arg \\ref VX_INPUT for [in]
 * \\arg \\ref VX_OUTPUT for [out]
 * \\arg \\ref VX_BIDIRECTIONAL for [in,out]
 *
 * When programming the parameters, use
 * \\arg \\ref VX_TYPE_IMAGE for a \\ref vx_image in the size field of \\ref vxGetParameterByIndex or \\ref vxSetParameterByIndex
 * \\arg \\ref VX_TYPE_BUFFER for a \\ref vx_buffer in the size field of \\ref vxGetParameterByIndex or \\ref vxSetParameterByIndex
 *
 * \\note All kernels in the lower level specification would be reflected here.
 * These names are prone to changing before the specification is complete.
 * \\ingroup group_kernel
 */
enum vx_kernel_e {
    /*!
     * \\brief The invalid kernel is used to indicate failure in relation to
     * some kernel operation (Get/Release).
     * \\details If the kernel is executed it shall always return an error.
     * The kernel has no parameters.
     * \\name "org.khronos.openvx.invalid"
     */
    VX_KERNEL_INVALID = 0,
""")

    # print all the unique kernel enums
    # make sure we don't write a given kernel twice
    writtenKernels = set()
    for node in vxNodeTable:
        printCKernelEnum(node, writtenKernels)

    # print the trailing boilerplate
    print("""    /*!
     * \\brief The last kernel used to delimit the range of the
     * kernel list. Should not be used by the users.
     */
    VX_KERNEL_MAX,

    /*!
     * \\brief The base value of the user extensions.
     */
    VX_KERNEL_EXTENSIONS = 0x3F000000,
};

#include "vx_operators.h"

#ifdef  __cplusplus
}
#endif

#endif  /* _OPEN_VISION_LIBRARY_KERNELS_H_ */

""")

    # clean up
    if filename:
        sys.stdout = tmpstdout
        outfile.close()

        
def checkNodeInfoItem(nodeinfo, i):
    "Verify that item 'i' of the given node description is properly formed."
    elements = ("name", "kernel", "shortname", "description")

    try:
        if not isinstance(nodeinfo[i], str):
            print("Malformed node information header:\n\t %s" % str(nodeinfo), sys.stderr)
            print("%s element \"%s\" specifier \"%s\" not a string." %
                  (ordinal[i], elements[i], str(nodeinfo[i])), sys.stderr)
            return False
    except:
        print("Malformed node information header:\n\t %s" % str(nodeinfo), sys.stderr)
        raise
        return False

    return True

def checkNodeInfo(nodeinfo):
    "Verify that the given node description is properly formed."
    try:  # unpack the information
        name, kernel, shortname, desc = nodeinfo
    except ValueError:
        print("Malformed node information header:\n\t %s" % str(nodeinfo), sys.stderr)
        print("Must be a 4-tuple of strings: (name, kernel, shortname, description)", sys.stderr)
        raise
        return False

    # check each element
    for i in range(0, 3):
        if not checkNodeInfoItem(nodeinfo, i): return False

    # check each line of the description
    try:
        for descline in desc.splitlines():
            if not isinstance(descline, str):
                print("Malformed node information header:\n\t %s" % str(nodeinfo),
                      sys.stderr)
                print("Description string %s unparseable." % desc,
                      sys.stderr)
                return False
    except:
        print("Malformed node information header:\n\t %s" % str(nodeinfo),
             sys.stderr)
        raise
        return False

    return True


def checkParameterSpec(param):
    """Check that the argument is a valid VX parameter specification.
    Must be a tuple of 3 string elements."""
    try:
        name, typename, desc = param
    except ValueError:
        print("Malformed node parameter spec:\n\t %s" % str(param),
              sys.stderr)
        print("Must be a 3-tuple of strings: (name, typename, description)",
              sys.stderr)
        raise
        return False

    try:
        for item in param:
            if not isinstance(item, str):
                print("Malformed parameter spec:\n\t %s" % str(param),
                      sys.stderr)
                print("Item \"%s\" not a string." % item, sys.stderr)
                return False
    except:
        print("Malformed parameter spec:\n\t %s" % str(param), sys.stderr)
        raise
        return False

    return True

def checkNodeTableEntry(node):
    """Validate a full table entry by unpacking its elements and
    checking each one."""
    try: # unpack
        nodeinfo, inputlist, outputlist = node
    except ValueError:
        print("Malformed node specifier:\n\t %s" % str(node), sys.stderr)
        print("Must be a 3-tuple: (node information, input list, output list)",
              sys.stderr)
        raise
        return False

    # check the top-level description
    if checkNodeInfo(nodeinfo) == False: return False

    # check all the parameter descriptions
    for paramlist in (inputlist, outputlist):
        for param in paramlist:
            if not checkParameterSpec(param): return False

    return True

def checkNodeTable(nodeTable):
    "Validate an entire node table by looping over its elements."
    for node in nodeTable:
        if checkNodeTableEntry(node) == False: return False
    return True
