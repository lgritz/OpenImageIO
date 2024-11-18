# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

cmake_minimum_required (VERSION 3.18.2)

project (OpenImageIO_Dependencies
         LANGUAGES CXX C)

message (STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
list (APPEND CMAKE_MODULE_PATH
      "${PROJECT_SOURCE_DIR}/.."
      "${PROJECT_SOURCE_DIR}/../modules"
      "${CMAKE_INSTALL_PREFIX}"
      )
message (STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}")

# Utilities
include(set_utils)

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
    set_cache (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist"
               "Installation location" PATH FORCE)
endif()
message (STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

set_option (BUILD_SHARED_LIBS "Build shared libraries" OFF VERBOSE FORCE)
set_cache (LOCAL_BUILD_SHARED_LIBS_DEFAULT ${BUILD_SHARED_LIBS}
           "Should local build make shared libraries")

include(check_is_enabled)
include(dependency_utils)
# include(ExternalProject)


set_option (BUILD_all "Build all dependencies unconditionally" OFF VERBOSE)

set (ALL_DEPS
        fmt
        Robinmap
        ZLIB
        Imath
        OpenEXR
        libjpeg-turbo
        libuhdr
        libdeflate
        TIFF
        # TODO: JXL
        # TODO: pugixml
        pybind11
        PNG
        Freetype
          pystring expat yaml-cpp minizip-ng
        OpenColorIO
        # TODO: TBB
        # TODO: DCMTK
        # TODO: FFmpeg
        # TODO: GIF
        # TODO: libheif
        # TODO: LibRaw
        # TODO: OpenJPEG
        # TODO: Ptex
        WebP
        # SKIP: OpenVDB
        # SKIP: OpenCV
        # SKIP: Qt6
    )

# Add a cache variable to enable every dependency
foreach (dep IN LISTS ALL_DEPS)
    set_option (BUILD_${dep} "Build ${dep} library" ${BUILD_all} VERBOSE)
endforeach()
# set_option (BUILD_ZLIB "Build ZLIB library" ${BUILD_all})

# Express dependencies between dependencies
if (BUILD_TIFF)
    set (BUILD_libdeflate ON)
    set (BUILD_ZLIB ON)
endif ()
if (BUILD_OpenColorIO)
    set (BUILD_pystring ON)
    set (BUILD_expat ON)
    set (BUILD_yaml-cpp ON)
    set (BUILD_minizip-ng ON)
endif ()

# include the builder for each dependency that is enabled
foreach (dep IN LISTS ALL_DEPS)
    if (BUILD_${dep})
        include (build_${dep})
        set (${dep}_ROOT ${CMAKE_INSTALL_PREFIX})
        message(STATUS "Set ${dep}_ROOT = ${${dep}_ROOT}")
    endif ()
endforeach()
