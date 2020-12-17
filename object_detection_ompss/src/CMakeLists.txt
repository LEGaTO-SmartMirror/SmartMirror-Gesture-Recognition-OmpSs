cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME YoloTRT)
project(${PROJECT_NAME} VERSION 0.2.0 LANGUAGES CXX C)

include("${CMAKE_SOURCE_DIR}/cmake/OmpSs.cmake")
INIT_OMPSS()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "Build Mode: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(CUDA)
find_package(TENSORRT PATHS cmake/modules REQUIRED)

include_directories(
    ${NOS6_INCLUDE_DIRS}
    ${TENSORRT_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    .
    YoloTensorRT
    YoloTensorRT/3rdParty/nvidia
)

file(GLOB cpp_src "*.cpp")
file(GLOB c_src "*.c")

message(STATUS "cpp_src: ${cpp_src}")
message(STATUS "c_src: ${c_src}")

ADD_LIBRARY_OMPSS(OpenCV_LIBS TENSORRT_LIBRARIES YoloLayerPlugin)
ADD_LIBRARY_DIR_OMPSS("${CMAKE_SOURCE_DIR}/YoloTensorRT")

set(MCC_OUTPUT_OBJ "main_TRT.o")
set(MCC_OUTPUT_BIN "YoloTRT_TEST.bin")

ADD_OMPSS_EXECUTABLE(MAIN ${c_src} OUTPUT_NAME ${MCC_OUTPUT_BIN} CXX_SRC ${cpp_src})

## TODO: Fix the deprecated calls
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")