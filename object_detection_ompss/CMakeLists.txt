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

# System level include directories, the SYSTEM options causes
# CMake to tell the compiler to ignore warnings within these
# header files
include_directories(SYSTEM
    ${TENSORRT_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    inc/3rdParty/nvidia
    inc/3rdParty/inipp
)

include_directories(
    inc
)

file(GLOB cpp_src "src/*.cpp")
file(GLOB c_src "src/*.c")

set(cpp_flags "${cpp_flags} -pedantic") # Enable pedantic warnings
set(cpp_flags "${cpp_flags} -Wall") # Enable all default g++ warning level
set(cpp_flags "${cpp_flags} -Wextra") # Enable additional warnings
set(cpp_flags "${cpp_flags} -Werror") # Treat all warnings as errors
set(cpp_flags "${cpp_flags} -Weffc++") # Enable warnings for violates of the effective C++ guidelines
set(cpp_flags "${cpp_flags} -Wunreachable-code") # Warn on unreachable code
set(cpp_flags "${cpp_flags} -Wunused-result") # Warn when the result of a function is not used
set(cpp_flags "${cpp_flags} -Wno-unused-function") # Disable the warning for unused functions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cpp_flags}") # Add all previous warning and standard defines to the CMake C++ flags

ADD_LIBRARY_OMPSS(OpenCV_LIBS TENSORRT_LIBRARIES YoloLayerPlugin)
ADD_LIBRARY_DIR_OMPSS("${CMAKE_SOURCE_DIR}/libs")
ADD_INCLUDE_DIR_OMPSS("${CMAKE_SOURCE_DIR}/inc")

set(MCC_OUTPUT_BIN "YoloTRT.bin")

ADD_EXECUTABLE_OMPSS(TARGET_NAME YoloTRT MAIN ${c_src} OUTPUT_NAME ${MCC_OUTPUT_BIN} CXX_SRC ${cpp_src})
COPY_BINARY_TO_HOST(YoloTRT "smartmirror5-2" "jasmin" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${MCC_OUTPUT_BIN})