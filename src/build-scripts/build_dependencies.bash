#!/usr/bin/env bash

# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO


defs=""
for arg in "$@"
do
    echo "Building $arg"
    defs+=" -DBUILD_${arg}=ON"
done

# defs+=" --debug-find"

if [[ "$defs" != "" ]]; then
    cmake -S src/cmake/dependencies -B $PWD/build/deps/depbuild \
          -D CMAKE_INSTALL_PREFIX=$PWD/build/deps/dist \
          -D OpenImageIO_Dependencies_LOCAL_INSTALL_DIR=$PWD/build/deps/dist \
          $defs
    cmake --build build/deps/depbuild
fi
