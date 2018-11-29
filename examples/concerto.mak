# 

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
#


_MODULE     := vx_xyz_lib
include $(PRELUDE)
TARGET      := vx_xyz_lib
TARGETTYPE  := library
CSOURCES    := vx_xyz_lib.c
SHARED_LIBS := openvx
include $(FINALE)

_MODULE     := xyz
include $(PRELUDE)
TARGET      := xyz
TARGETTYPE  := dsmo
DEFFILE     := xyz.def
CSOURCES    := vx_xyz_module.c
SHARED_LIBS := openvx
include $(FINALE)

_MODULE     := vx_example
include $(PRELUDE)
TARGET      := vx_example
TARGETTYPE  := exe
CSOURCES    := vx_graph_factory.c vx_factory_corners.c vx_factory_pipeline.c vx_factory_edge.c
IDIRS       := $(HOST_ROOT)/$(OPENVX_SRC)/include $(HOST_ROOT)/$(OPENVX_SRC)/extensions/include
STATIC_LIBS := vx_xyz_lib openvx-debug-lib openvx-helper
SHARED_LIBS := openvx
SYS_SHARED_LIBS := $(XML2_LIB)
include $(FINALE)

_MODULE     := vx_example_code
include $(PRELUDE)
TARGET      := vx_example_code
TARGETTYPE  := library
CSOURCES    := vx_imagepatch.c vx_delaygraph.c vx_super_res.c  vx_independent.c
CSOURCES    += vx_matrix_access.c vx_parameters.c vx_kernels.c 
CSOURCES    += vx_single_node_graph.c vx_introspection.c vx_multi_node_graph.c
CSOURCES    += vx_convolution.c vx_warps.c vx_callback.c vx_extensions.c
include $(FINALE)

ifneq (,$(findstring OPENVX_USE_TILING,$(SYSDEFS)))
_MODULE	    := openvx-tiling
include $(PRELUDE)
TARGET	    := openvx-tiling
TARGETTYPE  := dsmo
CSOURCES    := vx_tiling_add.c vx_tiling_alpha.c vx_tiling_gaussian.c vx_tiling_box.c vx_tiling_ext.c
SHARED_LIBS := openvx
include $(FINALE)

_MODULE	    := tiling_test
include $(PRELUDE)
TARGET	    := tiling_test
TARGETTYPE  := exe
CSOURCES    := vx_tiling_main.c
STATIC_LIBS := openvx-debug-lib openvx-helper
SHARED_LIBS := openvx
SYS_SHARED_LIBS := $(XML2_LIB)
include $(FINALE)
endif
