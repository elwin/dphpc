#!/usr/bin/env bash

set -euo pipefail

set -x
mpirun --version
"${CXX}" --version
cmake --version
set +x

echo "PATH=${PATH}"
echo "CXX=${CXX}"
echo "CXXFLAGS=${CXXFLAGS}"
echo "MAKEFLAGS=${MAKEFLAGS}"
echo "CMAKE_BUILD_TYPE=${BUILD_TYPE}"
