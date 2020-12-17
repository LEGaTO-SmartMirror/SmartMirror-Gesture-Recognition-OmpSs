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
