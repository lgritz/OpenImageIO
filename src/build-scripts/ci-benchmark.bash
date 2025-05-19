#!/usr/bin/env bash

# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO

BUILD_BIN_DIR=build/bin
if [[ "${RUNNER_OS}" == "Windows" ]] ; then
    BUILD_BIN_DIR+=/${CMAKE_BUILD_TYPE}
fi

ls build
ls $BUILD_BIN_DIR

# The unit tests by default don't do all the benchmark trials, to cut down
# on CI time. But in this case, we do want them to run full benchmarks.
# So set the special env variable to make that happen.
OIIO_CI_DO_BENCHMARK=1

mkdir -p build/benchmarks
for t in image_span_test simd_test ; do
    echo
    echo
    echo "$t"
    echo "========================================================"
    time ${BUILD_BIN_DIR}/$t > build/benchmarks/$t.out
    cat build/benchmarks/$t.out
    echo "========================================================"
    echo "========================================================"
    echo
done
