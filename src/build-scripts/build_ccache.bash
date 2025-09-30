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

CCACHE_VERSION=${CCACHE_VERSION:=4.12}
CCACHE_TAG=${CCACHE_TAG:=v${CCACHE_VERSION}}
LOCAL_DEPS_DIR=${LOCAL_DEPS_DIR:=${PWD}/ext}
CCACHE_INSTALL_DIR=${CCACHE_INSTALL_DIR:=${LOCAL_DEPS_DIR}/ccache}

if [[ `uname` == "Linux" && `uname -m` == "x86_64" ]] ; then
    mkdir -p ${CCACHE_INSTALL_DIR} || true
    mkdir -p ${LOCAL_DEPS_DIR}/bin || true
    pushd ${CCACHE_INSTALL_DIR}
    CCACHE_DESCRIPTOR="ccache-${CCACHE_VERSION}-linux-x86_64"
    curl --location "https://github.com/ccache/ccache/releases/download/${CCACHE_TAG}/${CCACHE_DESCRIPTOR}.tar.xz" -o ccache.tar.xz
    tar xJvf ccache.tar.xz
    cp ${CCACHE_INSTALL_DIR}/${CCACHE_DESCRIPTOR}/ccache ${LOCAL_DEPS_DIR}/bin
    popd
fi
