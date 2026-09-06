#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/src/build"
DIST_DIR="${SCRIPT_DIR}/dist/SDLPoP"

echo "=== Building SDLPoP for PSP ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

psp-cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_PRX=ON
make -j"$(sysctl -n hw.ncpu)"

echo "=== Packaging Release Bundle ==="
mkdir -p "${DIST_DIR}"
cp "${SCRIPT_DIR}/EBOOT.PBP" "${DIST_DIR}/EBOOT.PBP"
cp "${SCRIPT_DIR}/SDLPoP.ini" "${DIST_DIR}/SDLPoP.ini"
rm -rf "${DIST_DIR}/data"
cp -R "${SCRIPT_DIR}/data" "${DIST_DIR}/data"

echo "=== Build Complete! ==="
ls -lh "${DIST_DIR}/EBOOT.PBP"
