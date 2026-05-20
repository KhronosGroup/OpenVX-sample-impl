[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![codecov](https://codecov.io/gh/KhronosGroup/OpenVX-sample-impl/branch/openvx_1.3/graph/badge.svg)](https://codecov.io/gh/KhronosGroup/OpenVX-sample-impl)

<p align="center"><img width="30%" src="https://raw.githubusercontent.com/GPUOpen-ProfessionalCompute-Libraries/MIVisionX/master/docs/data/OpenVX_logo.png" /></p>

# OpenVX 1.3.2 Sample Implementation

<a href="https://www.khronos.org/openvx/" target="_blank">Khronos OpenVX™</a> is an open, royalty-free standard for cross platform acceleration of computer vision applications. OpenVX enables performance and power-optimized computer vision processing, especially important in embedded and real-time use cases such as face, body and gesture tracking, smart video surveillance, advanced driver assistance systems (ADAS), object and scene reconstruction, augmented reality, visual inspection, robotics and more.

This document outlines the purpose of this sample implementation as well as provide build and execution instructions.

## Contents

* [Purpose](#purpose)
* [Building and Executing](#building-and-executing)
  * [CMake](#cmake)
  * [Build.py Options](#buildpy-options)
  * [Concerto](#concerto)
* [Sample Build Instructions](#sample-build-instructions)
* [Conformance Test Modes](#conformance-test-modes)
* [Included Unit Tests](#included-unit-tests)
* [Debugging](#debugging)
* [Packaging and Installing](#packaging-and-installing)
* [CI/CD](#cicd)
* [Bug Reporting](#bug-reporting)

## Purpose

The purpose of this software package is to provide a sample implementation of the OpenVX 1.3.2 Specification that passes the conformance test. It is NOT intended to be a reference implementation. If there are any discrepancies with the OpenVX 1.3.2 specification, they are not intentional and the specification should take precedence. Many of the design decisions made in this sample implementation were motivated out of convenience rather than optimizing for performance. It is expected that vendor's implementations would choose to make different design choices based on their priorities, and the specification was written in such a way as to allow freedom to do so. Beyond the conformance tests, there was very limited testing, as this was not intended to be directly used as production software.

This sample implementation contains additional 'experimental' or 'internally proposed' features which are not included in OpenVX 1.3.2. Since these are not part of OpenVX, these are disabled by default in the build by using preprocessor definitions. These features may potentially be modified or may never be added to the OpenVX spec, and should not be relied on as such. Additional details on these preprocessor definitions can be found in the `BUILD_DEFINES` document in this same folder.

Future revisions of the OpenVX sample implementation may or may not be released, and Khronos is not actively maintaining a public open source sample implementation project.

The following is a summary of what this sample implementation IS and IS NOT:

**IS:**
* Passing OpenVX 1.3.2 conformance tests
* Implementing the full core API (context, image, graph, kernel, node, scalar, array, object array, pyramid, remap, distribution, threshold, LUT, matrix, convolution, delay, tensor, meta format)
* Implementing all standard vision and neural network kernels
* Supporting extensions: Import/Export (IX), Neural Networks (NN), User Data Object, NNEF Import Kernel, U1 (binary image)

**IS NOT:**
* A reference implementation
* Optimized
* Production ready
* Actively maintained by Khronos publicly

> **Note:** The Pipelining, Streaming, and Event Queue extension APIs are present as stubs (return `VX_ERROR_NOT_IMPLEMENTED`). They are included for API compatibility but do not have functional implementations.

## Building and Executing

The sample implementation contains two different build system options: cmake and concerto (non-standard makefile-based system). The build and execution instructions for each are shown below.

### CMake

#### Supported Systems

* Linux
* macOS (Darwin)
* Android
* Windows (Visual Studio or Cygwin)

#### Prerequisites

* Python 3 (tested with Python 3.8+)
* CMake 3.10 or higher (should be in PATH)
* GCC 4.3+ or Clang (Linux/macOS)
* Git submodules initialized (`git submodule update --init --recursive`)

##### macOS

* Xcode Command Line Tools (`xcode-select --install`). Apple Clang is used as the default compiler.
* Use `--os=Linux` with `Build.py` (macOS shares the same CMake code path as Linux).
* Libraries are built as `.dylib` instead of `.so`.
* Use `DYLD_LIBRARY_PATH` instead of `LD_LIBRARY_PATH` when running executables.

##### Windows

* Visual Studio 2013 or higher to create VS solution and use the VS compiler to build OpenVX and related projects (need DEVENV in PATH). Or Cygwin.

##### Android

* NDK toolchain. Set `ANDROID_NDK_TOOLCHAIN_ROOT` environment variable.

##### OpenCL (optional)

* Set `VX_OPENCL_INCLUDE_PATH` and `VX_OPENCL_LIB_PATH` environment variables.

##### XML (optional)

* libxml2 development package (e.g. `libxml2-dev` on Ubuntu).

#### Building

**Windows** -- from VS / Cygwin command prompt:

```shell
python3 Build.py --help
```

In case of Visual Studio solution, the default `CMAKE_GENERATOR` is `Visual Studio 12`. You can change it with the `--gen` option (`cmake --help` presents the supported generators).

**Linux** -- from shell:

```shell
python3 Build.py --help
```

The command above will present the available build options; please follow these options.

**VS solution** will be created in:

```
${OUTPUT_PATH}/build/${OS}/${ARCH}/OpenVX.sln
```

In order to build and install all the sample projects from VS, build the `INSTALL` project (Build.py triggers it by default).

**Makefiles** will be created in:

```
${OUTPUT_PATH}/build/${OS}/${ARCH}/${CONF}
```

In order to build and install all the sample projects, call `make install` (Build.py triggers it by default).

| Variable | Description |
|----------|-------------|
| `OUTPUT_PATH` | The path to output (default: root directory) |
| `OS` | `Win` / `Linux` / `Android` (Cygwin is equal to Linux) |
| `ARCH` | `x32` / `x64` |
| `CONF` | `Release` / `Debug` |

**Enable / Disable experimental options (optional):**

*Windows:*
1. Run the python script with `--build=false`
2. Open CMake GUI
   - Set the source code directory to the openvx root folder, where the root `CMakeLists.txt` is located
   - Set the build directory to `${OUTPUT_PATH}/build/${OS}/${ARCH}`
3. Select the options to enable / disable
4. Click the `Configure` button to update `CMakeCache.txt` (do not click `Generate`)
5. Re-run the python script (you can set `--build=true` or build from Visual Studio)

*Linux:*
1. Run the python script with `--build=false`
2. Navigate to `${OUTPUT_PATH}/build/${OS}/${ARCH}/${CONF}`
3. Run `make edit_cache`
4. Select the options to enable / disable
5. Press `c` to configure
6. Press `g` to generate the makefiles
7. Run `make install`

#### Install

The build process installs the OpenVX headers in `include/`, executables and libraries in `bin/`, and libs in `lib/` under:

```
${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}
```

#### Running

**Windows:**

Add `${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin` to `PATH`.

**Linux:**

```shell
export LD_LIBRARY_PATH=${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin
cd raw
${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin/vx_test
```

**macOS:**

```shell
export DYLD_LIBRARY_PATH=${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin
cd raw
${OUTPUT_PATH}/install/${OS}/${ARCH}/${CONF}/bin/vx_test
```

### Build.py Options

The `Build.py` script wraps CMake and provides command-line options for configuring the build. Run `python3 Build.py --help` for full usage.

#### General Options

| Option | Description | Default |
|--------|-------------|---------|
| `--os` | Operating system (`Linux` / `Windows` / `Android`) | *(required)* |
| `--arch` | Architecture (`32` / `64`) | `64` |
| `--conf` | Configuration (`Release` / `Debug`) | `Release` |
| `--c` | C compiler path | system default |
| `--cpp` | C++ compiler path | system default |
| `--gen` | CMake generator | `Visual Studio 12` (Windows), CMake default (Linux) |
| `--env` | Print supported environment variables | `False` |
| `--out` | Output path for build/install files | root directory |
| `--build` | Build and install targets | `True` |
| `--rebuild` | Clean rebuild (use when adding source files) | `False` |
| `--package` | Build packages | `False` |
| `--dump_commands` | Export compile commands for tooling (YCM, etc.) | `False` |

#### Conformance Feature Sets

| Option | CMake Define | Description |
|--------|-------------|-------------|
| `--conf_vision` | `OPENVX_CONFORMANCE_VISION=ON` | Vision conformance feature set |
| `--conf_nn` | `OPENVX_CONFORMANCE_NEURAL_NETWORKS=ON` | Neural Networks conformance feature set |
| `--conf_nnef` | `OPENVX_CONFORMANCE_NNEF_IMPORT=ON` | NNEF Import conformance feature set |
| `--enh_vision` | `OPENVX_USE_ENHANCED_VISION=ON` | Enhanced Vision feature set |

#### Official Extensions

| Option | CMake Define | Description |
|--------|-------------|-------------|
| `--ix` | `OPENVX_USE_IX=ON` | Import/Export extension |
| `--nn` | `OPENVX_USE_NN=ON` | Neural Network extension |
| `--pipelining` | `OPENVX_USE_PIPELINING=ON` | Pipelining extension |
| `--streaming` | `OPENVX_USE_STREAMING=ON` | Streaming extension |
| `--opencl_interop` | `OPENVX_USE_OPENCL_INTEROP=ON` | OpenCL Interop extension |
| `--u1` | `OPENVX_USE_U1=ON` | Binary (1-bit) image support |

#### Provisional Extensions

| Option | CMake Define | Description |
|--------|-------------|-------------|
| `--tiling` | `OPENVX_USE_TILING=ON` | Tiling extension |
| `--s16` | `OPENVX_USE_S16=ON` | Extended S16 support |

#### Experimental Features

| Option | CMake Define | Description |
|--------|-------------|-------------|
| `--f16` | `EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT=ON` | VX_TYPE_FLOAT16 support |
| `--venum` | `EXPERIMENTAL_USE_VENUM=ON` | Raspberry Pi 3B+ NEON target (forces 32-bit) |
| `--opencl` | `EXPERIMENTAL_USE_OPENCL=ON` | OpenCL target (forces 32-bit) |

> **Note:** Using `--venum`, `--tiling`, or `--opencl` automatically sets the architecture to 32-bit.

### Concerto

The following systems are supported by the concerto build:

* Linux (GCC or CLANG)
* Darwin/Mac OSX
* Windows NT (Cygwin or native Visual Studio 2013)

For each system, consider the following prerequisites:

#### 1. Linux

GCC is the default compiler. If Clang is desired, it can be chosen by setting the `HOST_COMPILER` environment variable to `CLANG`.

If using GCC, version 4.3.0 or above should be used.

#### 2. Darwin/OSX

Mac OSX users need to use either MacPorts or Fink (or other favorite package manager) to install the same set of packages above.

#### 3. Windows NT (Cygwin or native)

**Cygwin:**

It is recommended that Windows users install Cygwin to build OpenVX. Obtain and run the setup utility at http://cygwin.com/setup.exe.

In addition to the defaults, you MUST manually select the `gcc-core`, `make`, and `gcc-g++` packages in the Cygwin setup utility. All are in the "Devel" category. GCC version 4.3.0 or above should be used.

**Native MS Visual Studio 2013:**

Required packages to build:
* Microsoft Visual Studio 2013 (at least)
* A make utility compiled for Windows (examples):
  * `mingw32-make.exe` (http://www.mingw.org)
  * `gmake` from XDCTools (http://downloads.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/rtsc/)

For building the code, adding the path to `make.exe` to the `PATH` is required after the `vcvarsall.bat` batch file (which configures the environment for compiling with VC) is executed from a CMD window.

#### Building OpenVX using Concerto

Once the correct packages above are installed, the sample implementation can be built by typing `make` in the OpenVX installation directory (e.g., `openvx_sample`):

```shell
cd openvx_sample
make
```

Outputs are placed in:

```
out/$(TARGET_OS)/$(TARGET_CPU)/$(TARGET_BUILD)/
```

These variables are visible in the make output. This will be referred to as `TARGET_OUT`, though this may not be present in the actual environment (users could define this themselves).

In order to see a list and description of all make commands for concerto, type:

```shell
make help
```

#### Executing OpenVX using Concerto

The tests can be manually run as follows. In any environment, PATHs may need to be altered to allow loading dynamic modules. The `$(TARGET_OUT)` variable below should be replaced with the actual path to the output file where the libraries and executables are placed.

##### On Linux

`TARGET_OUT` is usually `out/LINUX/x86_64/release`

```shell
cd raw
LD_LIBRARY_PATH=../$(TARGET_OUT) ../$(TARGET_OUT)/vx_test
```

##### On Windows (Cygwin)

`TARGET_OUT` is usually `out/CYGWIN/X86/release`

```shell
cd raw
LD_LIBRARY_PATH=../$(TARGET_OUT) ../$(TARGET_OUT)/vx_test
```

##### On Windows (native)

`TARGET_OUT` is usually `out\Windows_NT\x86\release`

```bat
C:\> copy raw\*.* %TARGET_OUT%
C:\> pushd %TARGET_OUT% && vx_test.exe
```

##### On Mac OSX

`TARGET_OUT` is usually `out/DARWIN/x86_64/release`

```shell
cd raw
DYLD_LIBRARY_PATH=../$(TARGET_OUT) ../$(TARGET_OUT)/vx_test
```

## Sample Build Instructions

### Sample 1 - Build OpenVX 1.3.2 on Ubuntu 22.04

* Install prerequisites

```shell
sudo apt-get update
sudo apt-get install -y cmake git python3 gcc g++
```

* Git clone project with recursive flag to get submodules

```shell
git clone --recursive https://github.com/KhronosGroup/OpenVX-sample-impl.git
```

* Use `Build.py` script

```shell
cd OpenVX-sample-impl/
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision --conf_nn
```

* Build and run conformance

```shell
export OPENVX_DIR=$(pwd)/install/Linux/x64/Debug
export VX_TEST_DATA_PATH=$(pwd)/cts/test_data/
mkdir build-cts
cd build-cts
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

### Sample 2 - Build OpenVX 1.3.2 on macOS

* Ensure Xcode Command Line Tools are installed

```shell
xcode-select --install
```

* Git clone project with recursive flag to get submodules

```shell
git clone --recursive https://github.com/KhronosGroup/OpenVX-sample-impl.git
```

* Use `Build.py` script (use `--os=Linux` for macOS)

```shell
cd OpenVX-sample-impl/
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision --conf_nn
```

* Build and run conformance

```shell
export OPENVX_DIR=$(pwd)/install/Linux/x64/Debug
export VX_TEST_DATA_PATH=$(pwd)/cts/test_data/
mkdir build-cts
cd build-cts
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES="$OPENVX_DIR/bin/libopenvx.dylib;$OPENVX_DIR/bin/libvxu.dylib;pthread;dl;m" -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON ../cts/
cmake --build .
DYLD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

> **Note:** On macOS, libraries use the `.dylib` extension instead of `.so`, and `DYLD_LIBRARY_PATH` is used instead of `LD_LIBRARY_PATH`.

### Sample 3 - Build OpenVX 1.3.2 on Raspberry Pi

* Git clone project with recursive flag to get submodules

```shell
git clone --recursive https://github.com/KhronosGroup/OpenVX-sample-impl.git
```

* Use `Build.py` script

```shell
cd OpenVX-sample-impl/
python3 Build.py --os=Linux --venum --conf=Debug --conf_vision --enh_vision --conf_nn
```

* Build and run conformance

```shell
export OPENVX_DIR=$(pwd)/install/Linux/x32/Debug
export VX_TEST_DATA_PATH=$(pwd)/cts/test_data/
mkdir build-cts
cd build-cts
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

## Conformance Test Modes

The sample implementation supports several conformance test configurations. Below are the supported modes matching the CI pipeline. For all modes, first set up the environment:

```shell
export OPENVX_DIR=$(pwd)/install/Linux/x64/Debug
export VX_TEST_DATA_PATH=$(pwd)/cts/test_data/
```

> **Note:** When switching between modes, remove the previous install directory before rebuilding: `rm -rf install/Linux/x64/Debug`

> **Note:** On macOS, use `.dylib` instead of `.so` for library paths in `OPENVX_LIBRARIES`, replace `LD_LIBRARY_PATH` with `DYLD_LIBRARY_PATH`, and omit `rt` from the library list (e.g., `"...libopenvx.dylib;...libvxu.dylib;pthread;dl;m"`).

### Mode 1 - Vision Conformance

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision
mkdir build-cts-mode-1 && cd build-cts-mode-1
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

### Mode 2 - Vision & Enhanced Vision Conformance

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision
mkdir build-cts-mode-2 && cd build-cts-mode-2
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

### Mode 3 - Neural Network Conformance

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_nn
mkdir build-cts-mode-3 && cd build-cts-mode-3
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

### Mode 4 - NNEF Import Conformance

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_nnef
mkdir build-cts-mode-4 && cd build-cts-mode-4
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;$OPENVX_DIR/bin/libnnef-lib.a\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_NNEF_IMPORT=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

### Mode 5 - Vision, Enhanced Vision, Neural Net, Import/Export, & U1

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision --conf_nn --nn --ix --u1
mkdir build-cts-mode-5 && cd build-cts-mode-5
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_CONFORMANCE_NEURAL_NETWORKS=ON -DOPENVX_USE_NN=ON -DOPENVX_USE_IX=ON -DOPENVX_USE_U1=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

### Mode 6 - User Data Object

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug
mkdir build-cts-mode-6 && cd build-cts-mode-6
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_USE_USER_DATA_OBJECT=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

> **Note:** Build.py does not have a `--user_data_object` flag. To enable User Data Object support, use CMake directly with `-DOPENVX_USE_USER_DATA_OBJECT=ON`.

### Mode 7 - Vision, Enhanced Vision, Pipelining, & Streaming

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision --pipelining --streaming
mkdir build-cts-mode-7 && cd build-cts-mode-7
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_USE_PIPELINING=ON -DOPENVX_USE_STREAMING=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance
```

> **Note:** The Pipelining, Streaming, and Event Queue APIs are stub implementations that return `VX_ERROR_NOT_IMPLEMENTED`. Conformance tests for this mode are expected to fail.

## Included Unit Tests

The sample implementation comes with a few unit sanity tests that exercise some examples of the specification. The executables are called:

**`vx_test`**
* Main function is in source file: `sample/tests/vx_test.c`
* No arguments will run all included unit tests
* Argument will list the tests to be run
* Single tests can be run by specifying test number using `-t <testnum>`
* Expects to be run from the `raw/` directory (the program looks for image files in the execution directory)

**`vx_query`**
* Main function is in source file: `tools/query/vx_query.c`
* Queries the implementation and prints out details about all kernels

**`vx_example`**
* Main function is in source file: `examples/vx_graph_factory.c`
* This is an example of creating and running a graph using what is called a graph factory. This example is beyond the scope of the OpenVX specification, but is an example of how graph parameters can be used to abstract the details of a graph to clients.

**`vx_bug13510`**, **`vx_bug13517`**, **`vx_bug13518`**
* Exercise code exposing fixed bugs

## Debugging

To build in debug mode (this will output in the `out/.../debug` folder rather than `out/.../release`; if you defined `TARGET_OUT`, you'll have to change it):

```shell
export NO_OPTIMIZE=1
make
```

or

```shell
make TARGET_BUILD=debug
```

To enable traces (printfs/logs/etc), use either the mask (higher priority) or list of zones to enable. In the mask, the zones are the bit places. Express the mask as a hex number:

```shell
export VX_ZONE_MASK=0x<hexnumber>
```

or use the list as a comma delimited set of zone numbers (see `debug/vx_debug.h`):

```shell
export VX_ZONE_LIST=0,1,2,3,6,9,14
```

The list of mapping zones to bits can be found in `debug/vx_debug.h`.

If you want these variables as part of the command line:

```shell
unset VX_ZONE_MASK
unset VX_ZONE_LIST
make
cd raw
VX_ZONE_LIST=0,3,16 vx_test <options>
```

> **Note:** `VX_ZONE_MASK` will override `VX_ZONE_LIST`. If you have both set, only `VX_ZONE_MASK` is being seen by the implementation.

## Packaging and Installing

On Linux, the sample implementation for OpenVX is packaged after a make in:

```
out/LINUX/x86_64/openvx-*.deb
```

Installing DEB package:

```shell
dpkg-deb -i <path>/openvx-*.deb
```

## CI/CD

The project uses GitLab CI (`.gitlab-ci.yml`) with an `ubuntu:22.04` image. The CI pipeline:

1. Installs prerequisites: `cmake`, `make`, `git`, `python3`, `gcc`, `g++`
2. Builds OpenVX in both Release and Debug configurations
3. Runs 7 conformance test modes:
   - Mode 1: Vision
   - Mode 2: Vision & Enhanced Vision
   - Mode 3: Neural Networks
   - Mode 4: NNEF Import
   - Mode 5: Combined (Vision, Enhanced Vision, Neural Networks, Import/Export, U1)
   - Mode 6: User Data Object
   - Mode 7: Pipelining & Streaming (`allow_failure` -- stub implementation)

Git submodules are fetched automatically via `GIT_SUBMODULE_STRATEGY: recursive`.

## Bug Reporting

Although Khronos is not actively maintaining a public project of this sample implementation, bug reports can be reported back to the working group in an effort to make sure that we don't overlook related specification and implementation issues going forward.

If any bugs are found in the sample implementation, you can notify the Khronos working group via:
* File an [issue on the GitHub](https://github.com/KhronosGroup/OpenVX-sample-impl/issues)
* The OpenVX feedback forum: https://community.khronos.org/c/openvx
