# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

######################################################################
# GLFW by hand!
######################################################################

set_cache (glfw3_BUILD_VERSION 3.4 "GLFW version for local builds")
set (glfw3_GIT_REPOSITORY "https://github.com/glfw/glfw")
set (glfw3_GIT_TAG "${glfw3_BUILD_VERSION}")
set_cache (glfw3_GIT_COMMIT "7b6aead9fb88b3623e3b3725ebb42670cbe4c579"
           "commit hash to verify tag against")

# GLFW builds both X11 and Wayland support on Linux and picks one at
# runtime. Wayland needs extra development packages; do without it rather
# than fail if they are missing.
set (_glfw3_wayland_args "")
if (UNIX AND NOT APPLE)
    find_program (_glfw3_wayland_scanner wayland-scanner)
    find_package (PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules (_glfw3_wayland QUIET wayland-client xkbcommon)
    endif ()
    if (NOT _glfw3_wayland_scanner OR NOT _glfw3_wayland_FOUND)
        message (STATUS "Building GLFW for X11 only "
                        "(wayland-scanner, wayland-client or xkbcommon not found)")
        set (_glfw3_wayland_args -D GLFW_BUILD_WAYLAND=OFF)
    endif ()
endif ()

build_dependency_with_cmake(glfw3
    VERSION         ${glfw3_BUILD_VERSION}
    GIT_REPOSITORY  ${glfw3_GIT_REPOSITORY}
    GIT_TAG         ${glfw3_GIT_TAG}
    GIT_COMMIT      ${glfw3_GIT_COMMIT}
    CMAKE_ARGS
        -D BUILD_SHARED_LIBS=OFF
        -D CMAKE_POSITION_INDEPENDENT_CODE=ON
        -D CMAKE_INSTALL_LIBDIR=lib
        -D GLFW_BUILD_EXAMPLES=OFF
        -D GLFW_BUILD_TESTS=OFF
        -D GLFW_BUILD_DOCS=OFF
        -D GLFW_INSTALL=ON
        ${_glfw3_wayland_args}
    )

# Signal to caller that we need to find again at the installed location
set (glfw3_REFIND TRUE)
set (glfw3_REFIND_ARGS CONFIG)
set (glfw3_REFIND_VERSION ${glfw3_BUILD_VERSION})
