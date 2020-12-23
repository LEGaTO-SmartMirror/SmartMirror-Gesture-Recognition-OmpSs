# File: tensorrt-config.cmake
# Copyright (c) 2020 Florian Porrmann
# 
# MIT License
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

if(TENSORRT_FOUND)
  return()
endif()

include(FindPackageHandleStandardArgs)
find_package(CUDA)

message("CUDA_TOOLKIT_ROOT_DIR = ${CUDA_TOOLKIT_ROOT_DIR}")

find_path(TENSORRT_INCLUDE_DIRS NvInfer.h
  HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR} /usr/include
  PATH_SUFFIXES include)

find_path(TENSORRT_INCLUDE_DIRS NvInferPlugin.h
  HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES include)

find_library(NVINFER nvinfer
  HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)

find_library(NVINFER_PLUGIN nvinfer_plugin
  HINTS  ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)

find_library(NVPARSERS nvparsers
  HINTS  ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)

# /opt/dev/onnx-tensorrt/build

find_library(NVONNXPARSER nvonnxparser
  HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)

find_library(NVONNXPARSER_RUNTIME nvonnxparser_runtime
  HINTS  ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
  PATH_SUFFIXES lib lib64 lib/x64 lib/aarch64-linux-gnu)

if(CUDA_FOUND AND NVINFER AND NVINFER_PLUGIN AND NVPARSERS AND NVONNXPARSER)
    message(" == Found TensorRT == ")
    message("CUDA Libraries      : ${CUDA_LIBRARIES}")
    message("CUDA Headers        : ${CUDA_INCLUDE_DIRS}")
    message("nvinfer             : ${NVINFER}")
    message("nvinfer_plugin      : ${NVINFER_PLUGIN}")
    message("nvparsers           : ${NVPARSERS}")
    message("nvonnxparser        : ${NVONNXPARSER}")
    MESSAGE("TensorRT headers    : ${TENSORRT_INCLUDE_DIRS}")
    set(TENSORRT_LIBRARIES ${CUDA_LIBRARIES} ${NVINFER} ${NVINFER_PLUGIN} ${NVPARSERS} ${NVONNXPARSER})
endif()

find_package_handle_standard_args(TENSORRT DEFAULT_MSG TENSORRT_INCLUDE_DIRS TENSORRT_LIBRARIES)
