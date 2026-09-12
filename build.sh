#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/src/build"
DIST_DIR="${SCRIPT_DIR}/dist/SDLPoP-PSP"

echo "=== Building SDLPoP-PSP for PSP ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

psp-cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_PRX=ON
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
make -j"${NPROC}"

if [ ! -f "${SCRIPT_DIR}/data/res_dos.pak" ]; then
    echo "=== Building Resource Pack ==="
    python3 "${SCRIPT_DIR}/tools/build_pak.py"
fi

echo "=== Packaging Release Bundle ==="
mkdir -p "${DIST_DIR}"
cp -p "${SCRIPT_DIR}/EBOOT.PBP" "${DIST_DIR}/EBOOT.PBP"
cp -p "${SCRIPT_DIR}/SDLPoP.ini" "${DIST_DIR}/SDLPoP.ini"
if [ ! -d "${DIST_DIR}/data" ]; then
    cp -a "${SCRIPT_DIR}/data" "${DIST_DIR}/data"
else
    rsync -a --delete "${SCRIPT_DIR}/data/" "${DIST_DIR}/data/"
fi
mkdir -p "${DIST_DIR}/mods"
if [ -f "${SCRIPT_DIR}/mods/mods.txt" ]; then
    cp -p "${SCRIPT_DIR}/mods/mods.txt" "${DIST_DIR}/mods/mods.txt"
fi

echo "=== Build Complete! ==="
ls -lh "${DIST_DIR}/EBOOT.PBP"
