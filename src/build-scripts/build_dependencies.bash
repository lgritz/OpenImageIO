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


if [[ "$defs" != "" ]]; then
    cmake -S src/cmake/dependencies -B build/deps/depbuild -DCMAKE_INSTALL_PREFIX=build/deps/dist $defs
    cmake --build build/deps/depbuild
fi
