#!/usr/bin/env bash

# Utility script to download or build ccacheccache
#
# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

# Exit the whole script if any command fails.
set -ex

echo "Building ccache"
uname
echo "HOME=$HOME"

# Repo and branch/tag/commit of ccache to download if we don't have it yet
CCACHE_REPO=${CCACHE_REPO:=https://github.com/ccache/ccache}
CCACHE_VERSION=${CCACHE_VERSION:=4.12}
CCACHE_TAG=${CCACHE_TAG:=v${CCACHE_VERSION}}
LOCAL_DEPS_DIR=${LOCAL_DEPS_DIR:=${PWD}/ext}
# Where to put ccache repo source (default to the ext area)
CCACHE_SRC_DIR=${CCACHE_SRC_DIR:=${LOCAL_DEPS_DIR}/ccache}
# Temp build area (default to a build/ subdir under source)
CCACHE_BUILD_DIR=${CCACHE_BUILD_DIR:=${CCACHE_SRC_DIR}/build}
# Install area for ccache (default to ext/dist)
CCACHE_INSTALL_DIR=${CCACHE_INSTALL_DIR:=${PWD}/ext/dist}
CCACHE_CONFIG_OPTS=${CCACHE_CONFIG_OPTS:=}


if [[ `uname` == "Linux" && `uname -m` == "x86_64" ]] ; then
    mkdir -p ${CCACHE_SRC_DIR}
    pushd ${CCACHE_SRC_DIR}

    #
    # Try to download -- had trouble with this on runners
    #
    # mkdir -p ${LOCAL_DEPS_DIR}/bin
    # CCACHE_DESCRIPTOR="ccache-${CCACHE_VERSION}-linux-x86_64"
    # curl --location "${CCACHE_REPO}/releases/download/${CCACHE_TAG}/${CCACHE_DESCRIPTOR}.tar.xz" -o ccache.tar.xz
    # tar xJvf ccache.tar.xz
    # cp ${CCACHE_SRC_DIR}/${CCACHE_DESCRIPTOR}/ccache ${CCACHE_INSTALL_DIR}/bin

    # Clone ccache project from GitHub and build
    if [[ ! -e ${CCACHE_SRC_DIR}/.git ]] ; then
        echo "git clone ${CCACHE_REPO} ${CCACHE_SRC_DIR}"
        git clone ${CCACHE_REPO} ${CCACHE_SRC_DIR}
    fi

    echo "git checkout ${CCACHE_TAG} --force"
    git checkout ${CCACHE_TAG} --force

    if [[ -z $DEP_DOWNLOAD_ONLY ]]; then
        time cmake -S . -B ${CCACHE_BUILD_DIR} -DCMAKE_BUILD_TYPE=Release \
                   -DCMAKE_INSTALL_PREFIX=${CCACHE_INSTALL_DIR} \
                   ${CCACHE_CONFIG_OPTS}
        time cmake --build ${CCACHE_BUILD_DIR} --config Release --target install
    fi

    popd
    ls ${CCACHE_INSTALL_DIR}
    ls ${CCACHE_INSTALL_DIR}/bin
fi
