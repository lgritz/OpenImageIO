# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

######################################################################
# ZLIB by hand!
######################################################################

set_cache (ZLIB_BUILD_VERSION 1.3.1 "ZLIB version for local builds")
set_cache (ZLIB_GIT_REPOSITORY "https://github.com/madler/zlib" "Repo URL")
set_cache (ZLIB_GIT_TAG "v${ZLIB_BUILD_VERSION}" "Git tag to checkout")

set_if_not (LOCAL_BUILD_SHARED_LIBS_DEFAULT OFF)
set_cache (ZLIB_BUILD_SHARED_LIBS ${LOCAL_BUILD_SHARED_LIBS_DEFAULT}
           DOC "Should execute a local ZLIB build, if necessary, build shared libraries" ADVANCED)

# string (MAKE_C_IDENTIFIER ${ZLIB_BUILD_VERSION} ZLIB_VERSION_IDENT)

ExternalProject_Add(ZLIB
    PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/ZLIB
    GIT_REPOSITORY  ${ZLIB_GIT_REPOSITORY}
    GIT_TAG         ${ZLIB_GIT_TAG}
    CMAKE_ARGS
        ${BUILDER_COMMON_CMAKE_ARGS}
        -D BUILD_SHARED_LIBS=${ZLIB_BUILD_SHARED_LIBS}
        -D CMAKE_POSITION_INDEPENDENT_CODE=ON
        -D CMAKE_INSTALL_LIBDIR=lib
    )
