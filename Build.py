#

# Copyright (c) 2011-2017 The Khronos Group Inc.
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


import os
import sys
import subprocess
import shutil
from optparse import OptionParser


gProjName = "OpenVX"


class os_enum(object):
    Linux = 1
    Win = 2
    Android = 3

    @staticmethod
    def toString(eVal):
        if eVal == os_enum.Linux:
            return "Linux"
        elif eVal == os_enum.Win:
            return "Win"
        elif eVal == os_enum.Android:
            return "Android"
        return ""


class arch_enum(object):
    x64 = 1
    x32 = 2

    @staticmethod
    def toString(eVal):
        if eVal == arch_enum.x64:
            return "x64"
        elif eVal == arch_enum.x32:
            return "x32"
        return ""


class configuration_enum(object):
    Release = 1
    Debug = 2

    @staticmethod
    def toString(eVal):
        if eVal == configuration_enum.Release:
            return "Release"
        elif eVal == configuration_enum.Debug:
            return "Debug"
        return ""


def main():
    parser = OptionParser(usage='usage: %prog [options]', description = "Generate build make / sln files")
    parser.add_option("--os", dest="os", help="Set the operating system (Linux [For Linux or Cygwin]/ Windows / Android)", default='')
    parser.add_option("--arch", dest="arch", help="Set the architecture (32 / 64 bit) [Default 64]", default='64')
    parser.add_option("--conf", dest="conf", help="Set the configuration (Release / Debug) [Default Release]", default='Release')
    parser.add_option("--c", dest="c_compiler", help="Set the C compiler [Default: the default in system (Optional)]", default='')
    parser.add_option("--cpp", dest="cpp_compiler", help="Set the CPP compiler (Default: the default in system (Optional)]", default='')
    parser.add_option("--gen", dest="generator", help="Set CMake generator [Default: [WIN]:\"Visual Studio 12\" [Linux]:Cmake default gernerator]", default='')
    parser.add_option("--env", dest="env_vars", help="Print supported environment variable [Default False]", default='False')
    parser.add_option("--out", dest="out_path", help="Set the path to generated build / install files [Default root directory]", default='')
    parser.add_option("--build", dest="build", help="Build and install the targets (change to non 'true' value in order to generate solution / make files only) [Default True]", default='True')
    parser.add_option("--rebuild", dest="rebuild", help="Rebuild the targets (Use it when you add source file) [Default False]", default='False')
    parser.add_option("--package", dest="package", help="Build packages", default='False')
    parser.add_option('--dump_commands', dest='dump_commands', help='Add -DCMAKE_EXPORT_COMPILE_COMMANDS=ON for YCM and other tools', default=False, action='store_true')
    # Official extensions
    parser.add_option("--ix", dest="ix", help="Add -DOPENVX_USE_IX=ON to support the import-export extension", default=False, action='store_true')
    parser.add_option("--nn", dest="nn", help="Add -DOPENVX_USE_NN=ON to support the neural network extension", default=False, action='store_true')
    parser.add_option("--opencl", dest="opencl", help="Add -DOPENVX_USE_OPENCL_INTEROP=ON to support OpenVX OpenCL InterOp", default=False, action='store_true')
    # Provisional extensions
    parser.add_option("--tiling", dest="tiling", help="Add -DOPENVX_USE_TILING=ON to support the tiling extension", default=False, action='store_true')
    parser.add_option("--s16", dest="s16", help="Add -DOPENVX_USE_S16=ON to have an extended support for S16", default=False, action='store_true')
    # Experimental features
    parser.add_option("--f16", dest="f16", help="Add -DEXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT=ON to support VX_TYPE_FLOAT16", default=False, action='store_true')

    options, args = parser.parse_args()
    if options.env_vars != "False":
        print("ANDROID_NDK_TOOLCHAIN_ROOT - ANDROID toolchain installation path")
        return

    operatingSys = os_enum.Linux
    if options.os.lower() == "windows":
        operatingSys = os_enum.Win
    elif options.os.lower() == "android":
        operatingSys = os_enum.Android
    elif options.os.lower() != "linux":
        sys.exit("Error: Please define the OS (Linux / Windows / Android)")

    cmake_generator = options.generator
    if operatingSys == os_enum.Win and not cmake_generator:
        cmake_generator = "Visual Studio 12"

    arch = arch_enum.x64
    if options.arch == "32":
        arch = arch_enum.x32

    conf = configuration_enum.Release
    if options.conf.lower() == "debug":
        conf = configuration_enum.Debug

    userDir = os.getcwd()
    cmakeDir = os.path.dirname(os.path.abspath(__file__))
    outputDir = cmakeDir
    if options.out_path:
        outputDir = options.out_path
        if not os.path.isdir(outputDir):
            sys.exit("Error: " + outputDir + " is not directory")

    # chdir to buildRootDir
    os.chdir(outputDir)

    # create build directory
    if not os.path.isdir("build"):
        os.makedirs("build")
    os.chdir("build")
    if not os.path.isdir(os_enum.toString(operatingSys)):
        os.makedirs(os_enum.toString(operatingSys))
    os.chdir(os_enum.toString(operatingSys))
    if not os.path.isdir(arch_enum.toString(arch)):
        os.makedirs(arch_enum.toString(arch))
    os.chdir(arch_enum.toString(arch))
    if operatingSys != os_enum.Win:
        if not os.path.isdir(configuration_enum.toString(conf)):
            os.makedirs(configuration_enum.toString(conf))
        os.chdir(configuration_enum.toString(conf))

    # Delete the old build directory in case of rebuild
    if options.rebuild.lower() != "false" and len(os.listdir(os.getcwd())) > 0:
        rm_dir = os.getcwd()
        (head, tail) = os.path.split(os.getcwd())
        os.chdir(head)
        shutil.rmtree(rm_dir)
        os.makedirs(tail)
        os.chdir(tail)

    install_dir = os.path.join(outputDir, "install", os_enum.toString(operatingSys), arch_enum.toString(arch))
    if operatingSys == os_enum.Win:
        # Add \\${BUILD_TYPE} in order to support "Debug" \ "Release" build in visual studio
        install_dir += '\\${BUILD_TYPE}'
    else:
        install_dir = os.path.join(install_dir, configuration_enum.toString(conf))

    cmake_generator_command = ''
    # if the user set generator
    if cmake_generator:
        if (operatingSys, arch) == (os_enum.Win, arch_enum.x64) and not cmake_generator.endswith('Win64'):
            cmake_generator += ' Win64'
        cmake_generator_command = '-G "{}"'.format(cmake_generator)

    cmd = ['cmake']
    cmd += [cmakeDir]
    cmd += ['-DCMAKE_BUILD_TYPE=' + configuration_enum.toString(conf)]
    cmd += ['-DCMAKE_INSTALL_PREFIX=' + install_dir]
    if operatingSys == os_enum.Android:
        cmd += ['-DANDROID=1']
    if arch == arch_enum.x64:
        cmd += ['-DBUILD_X64=1']
    cmd += [cmake_generator_command]
    if options.c_compiler:
        cmd += ['-DCMAKE_C_COMPILER=' + options.c_compiler]
    if options.cpp_compiler:
        cmd += ['-DCMAKE_CXX_COMPILER=' + options.c_compiler]
    if options.package.lower() != 'false':
        cmd += ['-DBUILD_PACKAGES=1']
    if options.dump_commands:
        cmd += ['-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']
    if options.ix:
        cmd += ['-DOPENVX_USE_IX=ON']
    if options.nn:
        cmd += ['-DOPENVX_USE_NN=ON']
    if options.opencl:
        cmd += ['-DOPENVX_USE_OPENCL_INTEROP=ON']
#    if options.nn16:
#        cmd += ['-DOPENVX_USE_NN_16=ON']
    if options.tiling:
        cmd += ['-DOPENVX_USE_TILING=ON']
    if options.s16:
        cmd += ['-DOPENVX_USE_S16=ON']
    if options.f16:
        cmd += ['-DEXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT=ON']
    cmd = ' '.join(cmd)

    print( "" )
    print("-- ===== CMake command =====")
    print( "" )
    print(cmd)
    print( "" )

    subprocess.Popen(cmd, shell=True).wait()

    if options.build.lower() == "true":
        if operatingSys == os_enum.Win:
            tmp = '"%DEVENV%" {}.sln /build {} /project INSTALL'.format(gProjName, configuration_enum.toString(conf))
            print(tmp)
            subprocess.Popen(tmp, shell=True).wait()
        else:
            subprocess.Popen("make install -j", shell=True).wait()

    # chdir back to user directory
    os.chdir(userDir)


if __name__ == '__main__':
    main()
