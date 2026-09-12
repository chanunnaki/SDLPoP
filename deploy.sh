#!/usr/bin/env bash
# Deploy SDLPoP-PSP to local PPSSPP emulator and real hardware via nexus-b (ssh n)
# Follows the shared infrastructure in /Users/chan/code/psp/PSP-DEV-SETUP.md
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist/SDLPoP-PSP"
HOST="n"
FOLDER="SDLPoP-PSP"

if [ ! -f "${DIST_DIR}/EBOOT.PBP" ]; then
    echo "Dist package not found. Building first..."
    "${SCRIPT_DIR}/build.sh"
fi

echo "=== Deploying SDLPoP-PSP ==="

# 1. Local PPSSPP Emulator Deployment
PPSSPP_CAT_DIR="${HOME}/.config/ppsspp/PSP/GAME/CAT_Homebrew/${FOLDER}"
mkdir -p "${PPSSPP_CAT_DIR}"
cp "${DIST_DIR}/EBOOT.PBP" "${PPSSPP_CAT_DIR}/"
cp "${DIST_DIR}/SDLPoP.ini" "${PPSSPP_CAT_DIR}/"
rm -rf "${PPSSPP_CAT_DIR}/data"
cp -R "${DIST_DIR}/data" "${PPSSPP_CAT_DIR}/"
mkdir -p "${PPSSPP_CAT_DIR}/mods"
if [ -f "${DIST_DIR}/mods/mods.txt" ]; then
    cp -p "${DIST_DIR}/mods/mods.txt" "${PPSSPP_CAT_DIR}/mods/"
fi
echo "[✓] Deployed to PPSSPP Homebrew category: ${PPSSPP_CAT_DIR}"

PPSSPP_STANDARD_DIR="${HOME}/.config/ppsspp/PSP/GAME/${FOLDER}"
mkdir -p "${PPSSPP_STANDARD_DIR}"
cp "${DIST_DIR}/EBOOT.PBP" "${PPSSPP_STANDARD_DIR}/"
cp "${DIST_DIR}/SDLPoP.ini" "${PPSSPP_STANDARD_DIR}/"
rm -rf "${PPSSPP_STANDARD_DIR}/data"
cp -R "${DIST_DIR}/data" "${PPSSPP_STANDARD_DIR}/"
mkdir -p "${PPSSPP_STANDARD_DIR}/mods"
if [ -f "${DIST_DIR}/mods/mods.txt" ]; then
    cp -p "${DIST_DIR}/mods/mods.txt" "${PPSSPP_STANDARD_DIR}/mods/"
fi
echo "[✓] Deployed to PPSSPP standard Game dir: ${PPSSPP_STANDARD_DIR}"

# 2. Hardware Deployment (nexus-b via ssh n)
# Automounts at /mnt/psp/<unit>/PSP/GAME/ per PSP-DEV-SETUP.md
deploy_hw() {
    local unit="$1"
    local target_dir="/mnt/psp/${unit}/PSP/GAME/${FOLDER}"

    if ! ssh -o ConnectTimeout=3 "$HOST" "test -d /mnt/psp/${unit}/PSP/GAME" 2>/dev/null; then
        echo "[-] Skip ${unit} -- not mounted on ${HOST}"
        return
    fi

    echo "[*] Found ${unit} mounted on ${HOST}. Deploying to ${target_dir}..."
    ssh "$HOST" "mkdir -p '${target_dir}' '${target_dir}/mods'"
    
    # 1. Direct, instant deployment of binary, config, and packed resource bundles
    scp -q "${DIST_DIR}/EBOOT.PBP" "${DIST_DIR}/SDLPoP.ini" "${HOST}:${target_dir}/"
    if [ -f "${DIST_DIR}/mods/mods.txt" ]; then
        scp -q "${DIST_DIR}/mods/mods.txt" "${HOST}:${target_dir}/mods/"
    fi
    ssh "$HOST" "mkdir -p '${target_dir}/data'"
    for pak in res_dos.pak res_snes.pak res_snes_alt.pak; do
        if [ -f "${DIST_DIR}/data/${pak}" ]; then
            scp -q "${DIST_DIR}/data/${pak}" "${HOST}:${target_dir}/data/${pak}"
        fi
    done
    ssh "$HOST" "rm -f '${target_dir}/data/res.pak' '${target_dir}/data/res_x68.pak'"

    # 2. Clean up stale runtime cfg so new ini takes effect immediately
    ssh "$HOST" "rm -f '${target_dir}/SDLPoP.cfg'"

    # 4. Only transfer heavy assets if missing on the target
    if ! ssh "$HOST" "test -d '${target_dir}/data'" 2>/dev/null; then
        echo "[*] Initializing asset directory on ${unit} (one-time copy)..."
        rsync -a --inplace --exclude=".*" --exclude="*.DS_Store" \
            "${DIST_DIR}/data/" "${HOST}:${target_dir}/data/"
    fi

    echo "[✓] Successfully deployed to physical ${unit} -> ${FOLDER}"
}

deploy_hw "PSP-3000-MH"
deploy_hw "PSP-3000-WH"

echo "=== Deployment Complete ==="
