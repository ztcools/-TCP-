#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

CMAKE_BUILD_TYPE="${1:-Release}"

echo "========================================"
echo "  TCP Server Build Script"
echo "========================================"
echo "Build Type: ${CMAKE_BUILD_TYPE}"
echo "Build Dir: ${BUILD_DIR}"
echo ""

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}" || exit 1

echo "Running cmake..."
cmake "${SCRIPT_DIR}" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"

echo ""
echo "Running make..."
make -j"$(nproc)"

if [ $? -eq 0 ]; then
  echo ""
  echo "========================================"
  echo "  Build Successful!"
  echo "========================================"
  echo "Executable: ${BUILD_DIR}/bin/tcp_server"
  echo ""
  echo "Usage:"
  echo "  ${BUILD_DIR}/bin/tcp_server -h"
  echo "========================================"
else
  echo ""
  echo "========================================"
  echo "  Build Failed!"
  echo "========================================"
  exit 1
fi
