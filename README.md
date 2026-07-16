[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

<p align="center"><img width="30%" src="https://raw.githubusercontent.com/GPUOpen-ProfessionalCompute-Libraries/MIVisionX/master/docs/data/OpenVX_logo.png" /></p>

# OpenVX 1.3.2 Sample Implementation

<a href="https://www.khronos.org/openvx/" target="_blank">Khronos OpenVX™</a> is an open, royalty-free standard for cross platform acceleration of computer vision applications. OpenVX enables performance and power-optimized computer vision processing, especially important in embedded and real-time use cases such as face, body and gesture tracking, smart video surveillance, advanced driver assistance systems (ADAS), object and scene reconstruction, augmented reality, visual inspection, robotics and more.

This document outlines the purpose of this sample implementation as well as provide build and execution instructions.

## Contents

* [Purpose](#purpose)
* [Conformance & Extension Support](#conformance--extension-support)
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
* Supporting extensions: Import/Export (IX), Neural Networks (NN), User Data Object, NNEF Import Kernel, U1 (binary image), Pipelining, Streaming

**IS NOT:**
* A reference implementation
* Optimized
* Production ready
* Actively maintained by Khronos publicly

## Conformance & Extension Support

The tables below summarize which OpenVX 1.3.2 conformance feature sets and extensions are implemented in this sample and whether they are exercised by the bundled Conformance Test Suite (`cts/`). "Conformance status" reflects the CI pipeline (`.github/workflows/ci.yml`), where each listed suite runs as a required gate against the Debug build.

### Conformance Feature Sets (Profiles)

| Feature set | Build flag / CMake define | Implemented | Representative CTS suites | Conformance status |
|---|---|---|---|---|
| Vision (base) | `--conf_vision` / `OPENVX_CONFORMANCE_VISION` | Yes | `ColorConvert.*`, `Box3x3.*`, `vxAddSub.*`, `Scale.*`, `HarrisCorners.*`, `MinMaxLoc.*`, `GaussianPyramid.*`, … (full core-vision suite) | Passing |
| Enhanced Vision | `--enh_vision` / `OPENVX_USE_ENHANCED_VISION` | Yes | `TensorOp.*`, `HogFeatures.*`, `MatchTemplate.*`, `LBP.*`, `BilateralFilter.*`, `Nonmaxsuppression.*`, `Houghlinesp.*`, `ControlFlow.*` | Passing |
| Neural Networks | `--conf_nn` / `OPENVX_CONFORMANCE_NEURAL_NETWORKS` | Yes | `TensorNetworks.*`, `*NN*` | Passing |
| NNEF Import | `--conf_nnef` / `OPENVX_CONFORMANCE_NNEF_IMPORT` | Yes | `*NNEF*` | Passing |

### Official / KHR Extensions

| Extension | Build flag / CMake define | Implemented | Representative CTS suites | Conformance status |
|---|---|---|---|---|
| Import/Export (`vx_khr_ix`) | `--ix` / `OPENVX_USE_IX` | Yes | `ExtensionObject.*`, `Graph/ExportImport*`, `*IX*` | Passing |
| Neural Network (`vx_khr_nn`) | `--nn` / `OPENVX_USE_NN` | Yes | `*NN*`, `TensorNetworks.*` | Passing |
| User Data Object | `--userdataobj` / `OPENVX_USE_USER_DATA_OBJECT` | Yes | `UserDataObject.*` | Passing |
| Binary image / U1 | `--u1` / `OPENVX_USE_U1` | Yes | `vxBinOp1u.*`, `vxuBinOp1u.*` | Passing |
| Pipelining | `--pipelining` / `OPENVX_USE_PIPELINING` | Yes | `GraphPipeline.*` | Passing |
| Streaming | `--streaming` / `OPENVX_USE_STREAMING` | Yes | `GraphStreaming.*` | Passing |
| Event Queue | built with Pipelining / Streaming | Yes | `GraphPipeline.*`, `GraphStreaming.*` (indirect) | Passing |

### Compute Targets

The kernels can be executed by different back-end *targets*. The portable C reference model is always built; the others are optional and selected at build time. When an optional target is enabled it registers its kernels with the framework, and the **same** conformance suites run against it.

| Target | Build flag / CMake define | Implemented | Conformance status |
|---|---|---|---|
| C reference model (`c_model`) | *(default, always built)* | Yes | Passing — validated by the full CI pipeline |
| VENUM — ARM NEON (Raspberry Pi) | `--venum` / `EXPERIMENTAL_USE_VENUM` | Yes | Passing on Raspberry Pi for the Vision & Enhanced Vision suites (see [Sample 3](#sample-3---build-openvx-132-on-raspberry-pi)) |
| OpenCL | `--opencl` / `EXPERIMENTAL_USE_OPENCL` | Yes (experimental) | Not validated in CI |

> **Raspberry Pi / VENUM:** The VENUM target provides NEON-accelerated implementations of the Vision and Enhanced Vision kernels for ARM (tuned for the Raspberry Pi 3B+). It is made up of ~40 NEON kernels in `kernels/venum/` plus their OpenVX target wrappers in `sample/targets/venum/`, covering color conversion, channel ops, filters/morphology, arithmetic/bitwise, geometric transforms (scale/warp/remap), features (FAST9, Harris, HOG, LBP, `MatchTemplate`), statistics (min/max/loc, mean-stddev, histogram, integral), pyramids/optical flow, and tensor ops. Enabling `--venum` forces a 32-bit build. It builds and passes the Vision and Enhanced Vision conformance suites on Raspberry Pi hardware.

### Provisional & Experimental Feature Toggles (no dedicated CTS coverage)

These options are available as build toggles but have **no dedicated conformance suite** in the bundled CTS, so they are not validated by the conformance pipeline.

| Option | Build flag / CMake define | Implemented | Conformance status |
|---|---|---|---|
| Tiling (provisional) | `--tiling` / `OPENVX_USE_TILING` | Yes | Not covered by CTS |
| Extended S16 (provisional) | `--s16` / `OPENVX_USE_S16` | Yes | Not covered by CTS |
| OpenCL Interop | `--opencl_interop` / `OPENVX_USE_OPENCL_INTEROP` | Yes | Not covered by CTS |
| XML import/export | `OPENVX_USE_XML` | Yes | Not covered by CTS |
| FLOAT16 support | `--f16` / `EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT` | Yes | Not covered by CTS |

> **Note:** Neural Networks and NNEF Import conformance rely on test data shipped in `cts/test_data/`. Every conformance suite listed in the tables above runs as a required (non-`continue-on-error`) job in the current CI, so a failure indicates a real regression rather than an expected/unsupported case.

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

> **Note:** By default the sample implementation enables only the base **Vision** conformance feature set (`OPENVX_CONFORMANCE_VISION=ON`); every other feature set (Enhanced Vision, Neural Networks, NNEF Import) and extension (IX, NN, U1, Pipelining, Streaming, User Data Object, etc.) is off unless you enable it explicitly, either with the corresponding `Build.py` flag or `-DOPENVX_*=ON` on the CMake line. This mirrors the Conformance Test Suite defaults.

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
| `--userdataobj` | `OPENVX_USE_USER_DATA_OBJECT=ON` | User Data Object extension |
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

The sample implementation supports several conformance test configurations. The per-feature modes below are provided for **local, reproducible runs**; the CI pipeline itself uses a single all-features build fanned out into filtered slices (see [CI/CD](#cicd)). For all modes, first set up the environment:

```shell
export OPENVX_DIR=$(pwd)/install/Linux/x64/Debug
export VX_TEST_DATA_PATH=$(pwd)/cts/test_data/
```

> **Note:** As of the pinned `cts` submodule, the CTS enables only the base **Vision** feature set by default; every other conformance feature set (Enhanced Vision, Neural Networks, NNEF Import) and extension (IX, NN, U1, Pipelining, Streaming, User Data Object) is opt-in and must be turned on explicitly with the corresponding `-DOPENVX_*=ON` flag on the CTS `cmake` line, as shown in each mode below.

> **Note:** The current `cts` submodule stores its test data (`cts/test_data/`) as regular git files, so **Git LFS is not required** — a recursive clone/submodule checkout fetches the real images directly.

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
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;$OPENVX_DIR/bin/libnnef-lib.a\;pthread\;dl\;m\;rt\;stdc++ -DCMAKE_C_FLAGS="-I$(pwd)/../kernels/NNEF-Tools/parser/cpp/include" -DOPENVX_CONFORMANCE_NNEF_IMPORT=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance '--filter=*NNEF*'
```

> **Note:** The NNEF static library `libnnef-lib.a` is installed under `bin/` (not `lib/`) because the sample-impl install directories are set to `bin`. The CTS link line must also add `stdc++` and the NNEF parser include path (`kernels/NNEF-Tools/parser/cpp/include`), as shown above and in the CI workflow. If the sample-impl build fails with `'numeric_limits' is not a member of 'std'` on a modern GCC, add `--cpp_flags="-include limits"` to the `Build.py` command.

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
python3 Build.py --os=Linux --arch=64 --conf=Debug --userdataobj
mkdir build-cts-mode-6 && cd build-cts-mode-6
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_USE_USER_DATA_OBJECT=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance '--filter=UserDataObject.*'
```

> **Note:** The `Build.py` flag for this extension is `--userdataobj` (it maps to `-DOPENVX_USE_USER_DATA_OBJECT=ON`). The matching `-DOPENVX_USE_USER_DATA_OBJECT=ON` flag must also be passed on the CTS `cmake` line so the `UserDataObject.*` tests are compiled.

### Mode 7 - Vision, Enhanced Vision, & Pipelining

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision --pipelining
mkdir build-cts-mode-7 && cd build-cts-mode-7
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_USE_PIPELINING=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance '--filter=GraphPipeline.*'
```

> **Note:** As of the current tip of tree (ToT), Pipelining and Streaming are implemented, so the `GraphPipeline.*` and `GraphStreaming.*` conformance tests are expected to pass.

### Mode 8 - Vision, Enhanced Vision, Pipelining, & Streaming

```shell
python3 Build.py --os=Linux --arch=64 --conf=Debug --conf_vision --enh_vision --pipelining --streaming
mkdir build-cts-mode-8 && cd build-cts-mode-8
cmake -DOPENVX_INCLUDES=$OPENVX_DIR/include -DOPENVX_LIBRARIES=$OPENVX_DIR/bin/libopenvx.so\;$OPENVX_DIR/bin/libvxu.so\;pthread\;dl\;m\;rt -DOPENVX_CONFORMANCE_VISION=ON -DOPENVX_USE_ENHANCED_VISION=ON -DOPENVX_USE_PIPELINING=ON -DOPENVX_USE_STREAMING=ON ../cts/
cmake --build .
LD_LIBRARY_PATH=$OPENVX_DIR/bin:./lib ./bin/vx_test_conformance '--filter=GraphStreaming.*'
```

> **Note:** Streaming is now implemented on ToT, so the `GraphStreaming.*` tests are expected to pass. In CI this job is required to pass.

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

Continuous integration runs on **GitHub Actions** (`.github/workflows/ci.yml`) using an Ubuntu 22.04 environment. It runs on every pull request and on pushes to all branches, and is organized as a **two-stage, build-once/fan-out pipeline** rather than one rebuild per feature mode:

**Stage 1 — Build (both jobs gate Stage 2):**

1. Install prerequisites: `cmake`, `make`, `git`, `python3`, `gcc`, `g++`
2. `build` — compiles the sample implementation once in **Debug** with the full feature set enabled (`OPENVX_FEATURES`: Vision, Enhanced Vision, Neural Networks, NNEF Import, NN, IX, U1, User Data Object, Pipelining, Streaming), then builds the CTS against it and uploads a single self-contained artifact (`build-cts/` + `install/` + `cts/test_data/`).
3. `build-release` — a parallel **Release** build of the sample implementation that acts as a build gate.

**Stage 2 — Conformance test slices:**

Many granular jobs (each `needs: [build, build-release]`) download the Stage 1 artifact and run one `--filter` slice of the suite; no job rebuilds. Slices are grouped by area, for example:

- Framework: baseline/smoke, graph, user kernels
- Data objects and User Data Object
- Import/Export (IX)
- Pipelining (split into `fast` and `stress` slices because the tests run against the slower unoptimized Debug C-model) and Streaming
- Neural Networks and NNEF import
- Vision and Enhanced Vision functional groups (color/channel, filters, arithmetic, geometric, features, statistics, image ops, pyramid, tensor ops, control flow, etc.)

Git submodules are fetched recursively. Pipelining and Streaming are implemented on ToT and their jobs (`GraphPipeline.*` / `GraphStreaming.*`) are expected to pass.

> **Note:** A single all-features build serves every test slice, which is cheaper than rebuilding per feature; the trade-off is that features are exercised together rather than in isolation. The manually reproducible per-feature "Conformance Test Modes" below are provided for local runs and do **not** correspond one-to-one to CI jobs.

## Bug Reporting

Although Khronos is not actively maintaining a public project of this sample implementation, bug reports can be reported back to the working group in an effort to make sure that we don't overlook related specification and implementation issues going forward.

If any bugs are found in the sample implementation, you can notify the Khronos working group via:
* File an [issue on the GitHub](https://github.com/KhronosGroup/OpenVX-sample-impl/issues)
* The OpenVX feedback forum: https://community.khronos.org/c/openvx
