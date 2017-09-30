cmake_minimum_required(VERSION 2.8.1 FATAL_ERROR)

project(Arty)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

#find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

find_package(OpenMP QUIET)
if (OPENMP_FOUND)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS})
endif ()

if (NOT WIN32)
    add_definitions(-DCOLORIZE_LOG)
endif ()

set(CMAKE_CXX_STANDARD 11)
add_subdirectory(src)
