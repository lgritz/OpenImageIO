# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO


set_cache (Imath_BUILD_VERSION 3.1.10 "Imath version for local builds")
set_cache (Imath_GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/Imath" "Repo URL")
set_cache (Imath_GIT_TAG "v${Imath_BUILD_VERSION}" "Git tag to checkout")

set_if_not (LOCAL_BUILD_SHARED_LIBS_DEFAULT OFF)
set_cache (Imath_BUILD_SHARED_LIBS ${LOCAL_BUILD_SHARED_LIBS_DEFAULT}
           DOC "Should a local Imath build, if necessary, build shared libraries" ADVANCED)


ExternalProject_Add(Imath
    PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/Imath
    GIT_REPOSITORY  ${Imath_GIT_REPOSITORY}
    GIT_TAG         ${Imath_GIT_TAG}
    CMAKE_ARGS 
        ${BUILDER_COMMON_CMAKE_ARGS}
        -D BUILD_SHARED_LIBS=${Imath_BUILD_SHARED_LIBS}
        # Don't build unnecessary parts of Imath
        -D BUILD_TESTING=OFF
        -D IMATH_BUILD_EXAMPLES=OFF
        -D IMATH_BUILD_PYTHON=OFF
        -D IMATH_BUILD_TESTING=OFF
        -D IMATH_BUILD_TOOLS=OFF
        -D IMATH_INSTALL_DOCS=OFF
        -D IMATH_INSTALL_PKG_CONFIG=OFF
        -D IMATH_INSTALL_TOOLS=OFF
    )
