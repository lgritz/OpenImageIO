#!/usr/bin/env bash

# Utility script to download exiftool

# Exit the whole script if any command fails.
set -ex

echo "Building exiftool"
uname

: ${EXIFTOOL_VERSION:=12.49}
: ${LOCAL_DEPS_DIR:=${PWD}/ext}

curl --location https://exiftool.org/Image-ExifTool-${EXIFTOOL_VERSION}.tar.gz -o "${LOCAL_DEPS_DIR}/exiftool.tgz"
pushd ${LOCAL_DEPS_DIR}
tar xzf exiftool.tgz
export PATH=$PATH:${LOCAL_DEPS_DIR}/Image-ExifTool-${EXIFTOOL_VERSION}
popd
