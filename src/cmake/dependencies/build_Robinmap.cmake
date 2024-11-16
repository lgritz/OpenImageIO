# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

######################################################################
# Robinmap by hand!
######################################################################

set_cache (Robinmap_BUILD_VERSION 1.3.0 "Robinmap version for local builds")
set_cache (Robinmap_GIT_REPOSITORY "https://github.com/Tessil/robin-map" "Repo URL")
set_cache (Robinmap_GIT_TAG "v${Robinmap_BUILD_VERSION}" "Git tag to checkout")

ExternalProject_Add(Robinmap
    # VERSION         ${Robinmap_BUILD_VERSION}
    PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/Robinmap
    GIT_REPOSITORY  ${Robinmap_GIT_REPOSITORY}
    GIT_TAG         ${Robinmap_GIT_TAG}
    # CMAKE_ARGS
    )

# # Set some things up that we'll need for a subsequent find_package to work
# set (Robinmap_ROOT ${Robinmap_INSTALL_DIR})
# 
# # Signal to caller that we need to find again at the installed location
# set (Robinmap_REFIND TRUE)
# set (Robinmap_VERSION ${Robinmap_BUILD_VERSION})
