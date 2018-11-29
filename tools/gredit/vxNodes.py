
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

## \file vxNodes.py
## \author Frank Brill <brill@ti.com>

"""
This file contains the primary source table that defines the
interfaces to the node types that are supported in this implementation
of OpenVX.  It is expressed as a Python list of node descriptions.
Each node description consists of a Python 3-tuple of:

    (<node-description>, <input-list>, <output-list>)

The node description is a 4-tuple of strings:

    (<constructor>, <kernel>, <short-name>, <description>)

The constructor and kernel names should be just unadorned names; the
"vx" or "VX_KERNEL_" prefix and "Node" suffix will be added by the
code generator, so don't put them in this table.  The C header file
for defining the node constructors will be generated using the
function name "vx<constructor>Node".  The C header file defining the
kernel types will be generated with the enum value of
"VX_KERNEL_<kernel>".  The short name will be used by the GUI as-is,
and the text description will be included in all three places.  The
description can be multi-line; if so use the triple-quotes and place
the description text at the beginning of the lines with no indentation
(as is done for this comment you're reading now.)  Backslashes need to
be doubled if you want a backslash to appear in the output as per the
example below.

The input and output lists are Python lists of parameter descriptors.
Each parameter descriptor is a 3-tuple of:

    (<name>, <type>, <description>)

The "nodeTemplate" list below contains two example node descriptors
that can be used as examples.  One is a simple dummy node, and the
other is a complex node with a multi-line description.

The actual node table follows the example.  After the table is Python
code to validate the format of the node table.  After editing this
file, you should run it via the Python interpreter to make sure the
edited node table is properly formatted.
"""

# Template list containing one "dummy" node
nodeTemplate = [
# Node description
(("SimpleDummy", "SIMPLE_DUMMY", "DUMMY", "A brief textual description."),
  # Input list
 [("input1name", "vx_image", "Input1 description."),
  ("input2name", "vx_enum", "Input2 description."),],
  # Output list
 [("output1name", "vx_buffer", "Output1 description."),
  ("output2name", "vx_enum", "Output2 description.")]),
(("ComplexDummy", "COMPLEX_DUMMY", "CMPLXDUM",
"""
This is a fairly long multi-line textual description of a complex
node.  Comments can run on for multiple lines if they are enclosed in
triple quotes.  If any special characters are used, they still need to
be "escaped" using the backslash character.  In particular, if you
want to include an actual backslash character, e.g., for use in a
doxygen directive, you need two backslashes.  A reference to "vx_item"
needs to be written as \\ref vx_item for example.
"""),
  # Input list
 [("input1name", "vx_image", "Input1 description."),
  ("input2name", "vx_enum", "Input2 description."),],
  # Output list
 [("output1name", "vx_buffer", "Output1 description."),
  ("output2name", "vx_enum", "Output2 description.")]),
]

# The actual table of OpenVX nodes
vxNodeTable = [

(("CopyImage", "COPY_IMAGE", "COPYIMG", "The Copy Image Node."),
 [("input", "vx_image", "The input image.")],
 [("output", "vx_image", "The output image.")]),

(("CopyBuffer", "COPY_BUFFER", "COPYBUF", """The Copy Buffer Node."""),
 [("input", "vx_buffer", "The input buffer.")],
 [("output", "vx_buffer", "The output buffer.")]),

(("ColorConvert", "COLOR_CONVERT", "COLCONV",
"""The Color Conversion Node. Conversions are from the format of the
input to the output format."""),
 [("input", "vx_image", "The input image to convert from.")],
 [("output", "vx_image", "The output image to convert into.")]),

(("ChannelExtract", "CHANNEL_EXTRACT", "EXTRACT",
"""Extracts a specific color channel from the input image to the
output buffer."""),
 [("input", "vx_image", "The input image."),
  ("channelNum", "vx_enum", "The channel to extract.")],
 [("output", "vx_image", "The selected channel.")]),

(("Convolution", "IMAGE_CONVOLUTION", "CONVOLV", "The Convolution Node."),
 [("input", "vx_image", "The input image."),
  ("operator", "vx_enum", "The \\ref vx_conv_operator_e enumeration.")],
 [("output", "vx_image", "The convolved result.")]),

(("ConvolutionCustom", "IMAGE_CONVOLUTION", "CUSTCNV",
  "The Convolution Node for Customer Operator masks."),
 [("input", "vx_image", "The input image."),
  ("operator", "vx_image", "The user's custom operator.")],
 [("output", "vx_image", "The convolved result.")]),

(("Sobel3x3", "LINEAR_IMAGE_FILTER", "SOBEL",
  "The Sobel3x3 node with compile-time type-checked parameters."),
 [("input", "vx_image", "The input image.  This should be in VX_DF_IMAGE_Y800 format.")],
 [("output", "vx_image", "The output image.  This will be in VX_DF_IMAGE_Y800 format.")]),

(("Prewitt3x3", "LINEAR_IMAGE_FILTER", "PREWITT",
  "The Prewitt3x3 node with compile-time type-checked parameters."),
 [("input", "vx_image", "The input image.  This should be in VX_DF_IMAGE_Y800 format.")],
 [("output", "vx_image", "The output image.  This will be in VX_DF_IMAGE_Y800 format.")]),

(("Kroon3x3", "LINEAR_IMAGE_FILTER", "KROON",
  "The Kroon3x3 node with compile-time type-checked parameters."),
 [("input", "vx_image", "The input image.  This should be in VX_DF_IMAGE_Y800 format.")],
 [("output", "vx_image", "The output image.  This will be in VX_DF_IMAGE_Y800 format.")]),

(("Scharr3x3", "LINEAR_IMAGE_FILTER", "SCHARR",
  "The Scharr3x3 node with compile-time type-checked parameters."),
 [("input", "vx_image", "The input image.  This should be in VX_DF_IMAGE_Y800 format.")],
 [("output", "vx_image", "The output image.  This will be in VX_DF_IMAGE_Y800 format.")]),

(("Cross2x2", "LINEAR_IMAGE_FILTER", "CROSS",
  "The Cross2x2 node with compile-time type-checked parameters."),
 [("input", "vx_image", "The input image.  This should be in VX_DF_IMAGE_Y800 format.")],
 [("output", "vx_image", "The output image.  This will be in VX_DF_IMAGE_Y800 format.")]),

(("CustomFilter", "LINEAR_IMAGE_FILTER", "CFILTER",
  "The Custom Filter node with compile-time type-checked paramters."),
 [("input", "vx_image", "The input image.  This should be in VX_DF_IMAGE_Y800 format.")],
 [("output", "vx_image", "The output image.  This will be in VX_DF_IMAGE_Y800 format."),
  ("op_x", "vx_image", "The DxD \\f$G_x\\f$ matrix."),
  ("op_y", "vx_image", "The DxD \\f$G_y\\f$ matrix.")]),

(("NonLinearFilter", "NONLINEAR_IMAGE_FILTER", "NONLIN", "The NonLinear Filter node."),
 [("input", "vx_image", "The input image."),
  ("type", "vx_enum", "The type of nonlinear filter to apply.")],
 [("output", "vx_image", "The filtered result.")]),

(("Threshold", "THRESHOLD", "THRESH", "Set pixels above hi or below lo to zero."),
 [("input", "vx_image", "The input image."),
  ("lo", "vx_uint8", "The lower threshold."),
  ("hi", "vx_uint8", "The upper threshold.")],
 [("output", "vx_image", "The thresholded output image.")]),

(("ImageScale", "SCALING", "SCALE", "Scale the dimensions of an image."),
 [("input", "vx_image", "The input image."),
  ("scale_x", "vx_uint32", "Amount to scale in the x direction."),
  ("scale_y", "vx_uint32", "Amount to scale in the y direction.")],
 [("output", "vx_image", "The scaled output image.")]),

(("GeometricTransform", "GEOMETRIC_TRANSFORM", "GEOMTX",
  "Apply the given geometric transformation."),
  [("input", "vx_image", "The input image."),
   ("matrix", "vx_buffer", "The transform to apply.")],
  [("output", "vx_image", "The resulting image.")]),

(("CannyEdgeDetector", "CANNY_EDGE_DETECTOR", "CANNY",
  "Apply a Canny edge detector."),
  [("input", "vx_image", "The input image."),
   ("lo", "vx_uint8", "The lower threshold."),
   ("hi", "vx_uint8", "The upper threshold.")],
  [("output", "vx_image", "The output edge image.")]),

 (("ImageMinMaxLoc", "IMAGE_MIN_MAX_LOC", "MINMAX",
   "Determine the locations of the maxima and minima of the input."),
 [("input", "vx_image", "The input image.")],
 [("minima", "vx_image", "The minima image."),
  ("maxima", "vx_image", "The maxima image.")]),

 (("ImageMinima", "IMAGE_MIN", "MIN",
   "Determine the minimum value of an image."),
 [("input", "vx_image", "The input image.")],
 [("output", "vx_buffer", "The minima.")]),

 (("ImageMaxima", "IMAGE_MAX", "MAX",
   "Determine the maximum value of an image."),
 [("input", "vx_image", "The input image.")],
 [("output", "vx_buffer", "The maxima.")]),

 (("Histogram", "HISTOGRAM", "HIST",
   "Compute the histogram of an image."),
 [("input", "vx_image", "The input image."),
  ("numBins", "vx_uint32", "The number of bins to use.")],
 [("output", "vx_buffer", "The histogram.")]),

 (("ImagePyramid", "IMAGE_PYRAMID", "PYRAMID",
   "Produces a scale pyramid of image."),
 [("input", "vx_image", "The input image.")],
 [("outby2", "vx_image", "The input scaled by 2."),
  ("outby4", "vx_image", "The input scaled by 4."),
  ("outby8", "vx_image", "The input scaled by 8.")]),

(("HarrisCorners", "HARRIS_CORNERS", "HARRIS", "Find Harris corners in an image."),
 [("input", "vx_image", "The input image."),
  ("sens_thresh", "vx_uint32", "Sensitivity threshold.")],
 [("output", "vx_image", "The Harris corner image.")]),

(("IntegralImage", "INTEGRAL_IMAGE_8", "INTEGRL", "The Image Integral Node."),
 [("input", "vx_image", "The input image.")],
 [("output", "vx_image", "The output image.")]),

(("OpticalFlow", "OPTICAL_FLOW", "OPTFLOW",
"""The optical flow node.  This node computes the optical flow between
two input images (usually the 'previous' image and the 'current'
image), and also takes the list of corners in the 'previous' image as
input.  It returns the set of corners in the 'current' image (for use
as input to the next call), and a bunch of flow vectors."""),
  [("old_input", "vx_image", "The previous image."),
   ("new_input", "vx_image", "The current image."),
   ("old_corners", "vx_buffer", "The previous corners.")],
  [("corners", "vx_buffer", "The new corners."),
   ("points", "vx_buffer", "The resulting flow vectors.")]),

(("NormalizeVectors", "NORMALIZE_VECTORS", "VECNORM", "The vector normalization node."),
 [("input", "vx_buffer", "The input vectors.")],
 [("output", "vx_buffer", "The normalized vectors.")]),

(("FastCornersVectors", "FAST_CORNERS", "FAST", "The FAST corner vector node."),
 [("input", "vx_buffer", "The input buffer.")],
 [("output", "vx_image", "The FAST corner image."),
  ("points", "vx_buffer", "The FAST corner points.")]),

]


# Check that the table above is properly formatted.  Need to "run"
# this file after editing the table above.
import sys
from vxNodeUtils import checkNodeTable
if not checkNodeTable(vxNodeTable):
    print("Node table format error...exiting.", sys.stderr)
    exit(0)

