#!/bin/bash
set -ex
: "${CATCH_RELEASE:?CATCH_RELEASE is not set}"

cd /work
DIST="https://github.com/catchorg/Catch2/archive/refs/tags"
FILE="v${CATCH_RELEASE}.tar.gz"
wget "${DIST}/${FILE}"
tar -xzf "${FILE}"
cd "Catch2-${CATCH_RELEASE}"
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DCATCH_DEVELOPMENT_BUILD=OFF
cmake --build build/ --target install
cd /work
rm -rf "${FILE}" "Catch2-${CATCH_RELEASE}"
