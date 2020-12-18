cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME YoloTRT)
project(${PROJECT_NAME} VERSION 0.2.0 LANGUAGES CXX C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include("cmake/OmpSs.cmake")
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
    ${TENSORRT_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    inc
    inc/3rdParty/nvidia
    inc/3rdParty/inipp
)

file(GLOB cpp_src "src/*.cpp")
file(GLOB c_src "src/*.c")

ADD_LIBRARY_OMPSS(OpenCV_LIBS TENSORRT_LIBRARIES YoloLayerPlugin)
ADD_LIBRARY_DIR_OMPSS("${CMAKE_SOURCE_DIR}/libs")
ADD_INCLUDE_DIR_OMPSS("${CMAKE_SOURCE_DIR}/inc")

set(MCC_OUTPUT_BIN "YoloTRT.bin")

ADD_EXECUTABLE_OMPSS(TARGET_NAME YoloTRT MAIN ${c_src} OUTPUT_NAME ${MCC_OUTPUT_BIN} CXX_SRC ${cpp_src})
COPY_BINARY_TO_HOST(YoloTRT "smartmirror5-2" "jasmin" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${MCC_OUTPUT_BIN})

## TODO: Fix the deprecated calls
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
