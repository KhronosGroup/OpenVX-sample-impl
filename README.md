[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Build Status](https://travis-ci.com/KhronosGroup/OpenVX-sample-impl.svg?branch=openvx_1.2)](https://travis-ci.com/KhronosGroup/OpenVX-sample-impl)
[![codecov](https://codecov.io/gh/KhronosGroup/OpenVX-sample-impl/branch/openvx_1.3/graph/badge.svg)](https://codecov.io/gh/KhronosGroup/OpenVX-sample-impl)

<p align="center"><img width="60%" src="https://upload.wikimedia.org/wikipedia/en/thumb/d/dd/OpenVX_logo.svg/1920px-OpenVX_logo.svg.png" /></p>

# OpenVX 1.3 Sample Implementation

<a href="https://www.khronos.org/openvx/" target="_blank">Khronos OpenVX™</a> is an open, royalty-free standard for cross platform acceleration of computer vision applications. OpenVX enables performance and power-optimized computer vision processing, especially important in embedded and real-time use cases such as face, body and gesture tracking, smart video surveillance, advanced driver assistance systems (ADAS), object and scene reconstruction, augmented reality, visual inspection, robotics and more.

This document outlines the purpose of this sample implementation as well as provide build and execution instructions.

## CONTENTS:

* [Purpose](#purpose)
* [Building And Executing](#building-and-executing)
  * [CMake](#cmake)
  * [Concerto](#concerto)
* [Sample Build Instructions](#sample-build-instructions)
* [Included Unit Tests](#included-unit-tests)
* [Debugging](#debugging)
* [Packaging And Installing](#packaging-and-installing)
* [Bug Reporting](#bug-reporting)

## Purpose

The purpose of this software package is to provide a sample implementation of the OpenVX 1.3 Specification that passes the conformance test. It is NOT intended to be a reference implementation.  If there are any discrepancies with the OpenVX 1.3 specification, they are not intentional and the specification should take precedence.  Many of the design decisions made in this sample implementation were motivated out of convenience rather than optimizing for performance.  It is expected that vendor's implementations would choose to make different design choices based on their priorities, and the specification was written in such a way as to allow freedom to do so. Beyond the conformance tests, there was very limited testing, as this was not intended to be directly used as production software.

This sample implementation contains additional 'experimental' or 'internally proposed' features which are not included in OpenVX 1.3.  Since these are not part of OpenVX, these are disabled by default in the build by using preprocessor definitions.  These features may potentially be modified or may never be added to the OpenVX spec, and should not be relied on as such. Additional details on these preprocessor definitions can be found in the BUILD_DEFINES document in this same folder.

Future revisions of the OpenVX sample implementation may or may not be released, and Khronos is not actively maintaining a public open source sample implementation project.

The following is a summary of what this sample implementation IS and IS NOT:

**IS**:
* passing OpenVX 1.3 conformance tests

**IS NOT**:
* a reference implementation
* optimized
* production ready
* actively maintained by Khronos publically

## Building And Executing

The sample implementation contains two different build system options:
cmake and concerto (non-standard makefile-based system). The build and execution instructions for each are shown below.

### CMake

#### Supported systems:

* Linux
* Android
* Windows (Visual studio or Cygwin)

#### Pre-requisite:

* python 2.x (Tested with python 2.7)
* CMAKE 2.8.12 or higher. (should be in PATH)

##### Windows:
* Visual studio 12 (2013) or higher in order to create VS solution and use VS compiler to build OpenVX and related projects. (Need DEVENV in PATH) Or Cygwin.

##### Android:
* NDK tool chain.

#### Building:

    Windows:
    --------
    From VS / Cygwin command prompt:

    > python Build.py --help
	
    In case of Visual Studio solution, the default CMAKE_GENERATOR is "Visual Studio 12", you can change it with --gen option. (cmake --help present the supported generators)

    Linux:
    ------
    From shell:

    > python Build.py --help

    The command above will present the available build options, please follow these options.

    VS solution will create in:
    ---------------------------
    ${OUTPUT_PATH}/build/${OS}/${ARCH}/OpenVX.sln

    In order to build and install all the sample projects from VS, build the 'INSTALL' project. (Build.py trigger it by default)

    Make files will create in:
    --------------------------
    ${OUTPUT_PATH}/build/${OS}/${ARCH}/${CONF}

    In order to build and install all the sample projects, call to 'make install'. (Build.py trigger it by default)

    OUTPUT_PATH - The path to output, default the root directory.
    OS - Win / Linux / Android. (Cygwin is equal to Linux)
    ARCH - 32 / 64
    CONF - Release / Debug.
	
    Enable / Disable experimental options (Optional):
    -------------------------------------------------
	Windows:
	--------
        (1) Run the python script, set --build=false
        (2) Open cmake GUI
            (2.1) Set the source code directory to the openvx root folder, where the root CMakeLists.txt is located
            (2.2) Set the build directory to ${OUTPUT_PATH}/build/${OS}/${ARCH}
        (3) Select the options to enable / disable
        (4) Click the 'Configure' button in order to update the "CMakeCache.txt" file (Do not click on 'Generate' button)
        (5) Re-run the python script. (You can set --build=true or build from Visual Studio)
		
        Linux:
        ------
        (1) Run the python script, set --build=false
        (2) Navigate to ${OUTPUT_PATH}/build/${OS}/${ARCH}/${CONF}
        (3) > make edit_cache
        (4) Select the options to enable / disable
        (5) click 'c' to configure
        (6) click 'g' to generate the make files
        (7) > make install

#### Install:

    The build process installs the OpenVX headers in 'include' folder / executables and libraries in 'bin' folder / libs in 'lib' folder under:
    ${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}

##### Running:

    Windows:
    --------
    Add ${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin to PATH

    Linux:
    ------
    Set LD_LIBRARY_PATH ${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin

    $ cd raw
    $ ${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin/vx_test

### Concerto

The following systems are supported by the concerto build:
* Linux (GCC or CLANG)
* Darwin/Mac OSX
* Windows NT (Cygwin or native Visual Studio 2013)

For each system, consider the following prerequisites:

    1. Linux
    --------
    gcc is the default compiler.  If clang is desired, then it can be chosen
    by setting the HOST_COMPILER environment variable to CLANG.

    If using gcc, version 4.3.0 or above should be used.

    2. Darwin/OSX
    -------------
    Mac OSX Users need to use either MacPorts or Fink (or other favorite
    package manager) to install the same set of packages above.

    3. Windows NT (Cygwin or native)
    -----------------------------

    (Cygwin)
    --------
    It is recommended that Windows users install Cygwin to build OpenVX.
    Obtain and run the setup utility at:

        http://cygwin.com/setup.exe

    In addition to the defaults, you MUST manually select the gcc-core,
    make, and gcc-g++ packages in the cygwin setup utility.  All are in
    the "Devel" category. gcc version 4.3.0 or above should be used.

    (native MS Visual Studio 2013)
    --------
    Required packages to build:
        - Microsoft Visual Studio 2013 (at least)
        - A make utility compiled for windows (examples):
          - mingw32-make.exe (http://www.mingw.org)
          - gmake from XDCTools (http://downloads.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/rtsc/)

    For building the code, adding the path to "make.exe" to
    the PATH is required after the "vcvarsall.bat" batch file (which
    configures the environment for compiling with VC) is executed from a
    CMD window.

#### Building OpenVX using Concerto

Once the correct packages above are installed, the sample implementation can be built by typing "make" in the OpenVX installation directory (e.g., "openvx_sample").

    $ cd openvx_sample
    $ make

Outputs are placed in 

    out/$(TARGET_OS)/$(TARGET_CPU)/$(TARGET_BUILD)/
    
These variables are visible in the make output. This will be referred to as TARGET_OUT, though this may not be present in the actual environment (users could define this themselves).

In order to see a list and description of all make commands for concerto,
type:

    $ make help


#### Executing OpenVX using Concerto

The tests can be manually run as follows. In any environment, PATHs may need to be altered to allow loading dynamic modules. The $(TARGET_OUT) variable below should be replaced with the actual path to the output file where the libraries and executables are placed, as per the description in section 1: BUILD.

##### On Linux

TARGET_OUT is usually out/LINUX/x86_64/release

    $ cd raw
    $ LD_LIBRARY_PATH=../$(TARGET_OUT) ../$(TARGET_OUT)/vx_test

##### On Windows (Cygwin)

TARGET_OUT is usually

    out/CYGWIN/X86/release

Commands:

    $ cd raw
    $ LD_LIBRARY_PATH=../$(TARGET_OUT) ../$(TARGET_OUT)/vx_test

##### On Windows (native)

TARGET_OUT is usually

    out\Windows_NT\x86\release

Commands:

    C:\> copy raw\*.* %TARGET_OUT%
    C:\> pushd %TARGET_OUT% && vx_test.exe

##### On Mac OSX (from openvx_sample)

TARGET_OUT is usually

    out/DARWIN/x86_64/release

Commands:

    $ cd raw
    $ DYLD_LIBRARY_PATH=../$(TARGET_OUT) ../$(TARGET_OUT)/vx_test

## Sample Build Instructions

### Sample 1 - Build OpenVX 1.3 on Ubuntu 18.04

* Git Clone project with recursive flag to get submodules
````
git clone --recursive https://github.com/KhronosGroup/OpenVX-sample-impl.git
````
* Use `Build.py` script
````
cd OpenVX-sample-impl/
python Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision --conf_nn
````
* Build and run conformance
````
export OPENVX_DIR=$(pwd)/install/Linux/x64/Debug
export VX_TEST_DATA_PATH=$(pwd)/cts/test_data/
mkdir build-cts
cd build-cts
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=./lib ./bin/vx_test_conformance
````

### Sample 2 - Build OpenVX 1.3 on Raspberry Pi

* Git Clone project with recursive flag to get submodules
````
git clone --recursive https://github.com/KhronosGroup/OpenVX-sample-impl.git
````
* Use `Build.py` script
````
cd OpenVX-sample-impl/
python Build.py --os=Linux --venum --conf=Debug --conf_vision --enh_vision --conf_nn
````
* Build and run conformance
````
export OPENVX_DIR=$(pwd)/install/Linux/x32/Debug
export VX_TEST_DATA_PATH=$(pwd)/cts/test_data/
mkdir build-cts
cd build-cts
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=./lib ./bin/vx_test_conformance
````

## Included Unit Tests

The sample implementation comes with a few unit sanity tests that exercises
some examples of the specification.  The executables are called:

**vx_test:**
* main function is in source file: openvx_sample/tests/vx_test.c
* No arguments will run all included unit tests.
* Argument will list the tests to be run
* Single tests can be run by specifying test number using -t <testnum>
* Expects to be run from the raw directory (the program looks for image files in the execution directory).

**vx_query:**
* Main function is in source file: openvx_sample/tools/query/vx_query.c
* Queries the implementation and prints out details about all kernels

**vx_example:**
* Main function is in source file: openvx_sample/examples/vx_graph_factory.c
* This is an example of creating and running a graph using what is called a graph factory.  This example is beyond the scope of the openvx specification, but is an example of how graph parameters can be use to abstract the details of a graph to clients.

vx_bug13510:
vx_bug13517:
vx_bug13518:
    - Exercise code exposing fixed bugs.

## Debugging

To build in debug mode (this will output in the out/.../debug folder rather
than out/.../release, thus if you defined TARGET_OUT, you'll have to change it. 

    $ export NO_OPTIMIZE=1
    $ make

or 

    $ make TARGET_BUILD=debug

To enable traces (printfs/logs/etc), use either the mask (higher priority) or
list of zones to enable. In the mask, the zones are the bit places. Express the
mask as a hex number.

    $ export VX_ZONE_MASK=0x<hexnumber>
    
or use the list as a comma delimited set of zone numbers (see 'sample/include/vx_debug.h')

    $ export VX_ZONE_LIST=0,1,2,3,6,9,14

The list of mapping zones to bits can be found in openvx_sample/debug/vx_debug.h

If you want these variable as part of the command line:

    $ unset VX_ZONE_MASK
    $ unset VX_ZONE_LIST
    $ make
    $ cd raw
    $ VX_ZONE_LIST=0,3,16 vx_test <options>

Note: VX_ZONE_MASK will override VX_ZONE_LIST. So if you have both set, only
VX_ZONE_MASK is being seen by the implementation. 

Now run your tests again.


## Packaging And Installing

On Linux, the sample implementation for OpenVX is packaged after a make in

    out/LINUX/x86_64/openvx-*.deb

Installing DEB package

    $ dpkg-deb -i <path>/openvx-*.deb


## Bug Reporting

Although Khronos is not actively maintaining a public project of this sample implementation, bug reports can be reported back to the working group in an effort to make sure that we don't overlook related specification and implementation issues going forward.

If any bugs are found in the sample implementation, you can notify the Khronos working group via 
* File an [issue on the GitHub](https://github.com/KhronosGroup/OpenVX-sample-impl/issues)
* The OpenVX feedback forum: https://community.khronos.org/c/openvx
