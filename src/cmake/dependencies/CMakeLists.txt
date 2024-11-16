# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

cmake_minimum_required (VERSION 3.18.2)

project (OpenImageIO_Dependencies
         LANGUAGES CXX C)

list (APPEND CMAKE_MODULE_PATH
      "${PROJECT_SOURCE_DIR}/.."
      )

# Utilities
include(set_utils)

include(ExternalProject)

option (VERBOSE "Print lots of messages while compiling" ON)
set (CMAKE_MESSAGE_LOG_LEVEL $<IF:$<BOOL:${VERBOSE}>,"VERBOSE","STATUS">
        CACHE STRING "CMake log level to display")

set_cache (CMAKE_CXX_STANDARD 17 "C++ standard to use")
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
endif ()
message (STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
set_if_not (CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist" CACHE PATH
         "Installation location" FORCE)
endif()
message (STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

set (prefix_paths ${CMAKE_INSTALL_PREFIX} ${CMAKE_PREFIX_PATH})
set (BUILDER_COMMON_CMAKE_ARGS
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -D CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        # -D CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
        -D CMAKE_IGNORE_PREFIX_PATH=${CMAKE_IGNORE_PREFIX_PATH}
        -D CMAKE_FIND_FRAMEWORK=${CMAKE_FIND_FRAMEWORK}
        -D CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -D CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -D CMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -D CMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE}
        -D CMAKE_COMPILE_WARNING_AS_ERROR=OFF
        -D BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
        -D CMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
        -D CMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
        -D CMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
        -D CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    )

set_option (BUILD_all "Build all dependencies unconditionally" OFF VERBOSE)

set (ALL_DEPS
        ZLIB
        Imath
        OpenEXR
    )

# set_cache (BUILD_Imath OFF "Build Imath library")
# if (BUILD_Imath OR BUILD_all)
#     include (${PROJECT_SOURCE_DIR}/build_Imath.cmake)
# endif ()

foreach (dep IN LISTS ALL_DEPS)
    set_cache (BUILD_${dep} ${BUILD_all} "Build ${dep} library" VERBOSE)
    if (BUILD_${dep} OR BUILD_all)
        include (${PROJECT_SOURCE_DIR}/build_${dep}.cmake)
        set (${dep}_ROOT ${CMAKE_INSTALL_PREFIX})
        message(STATUS "Set ${dep}_ROOT = ${${dep}_ROOT}")
    endif ()
endforeach()
