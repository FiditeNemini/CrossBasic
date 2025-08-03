#!/usr/bin/env bash
set -euo pipefail

# --- Configuration ---
SRC="./CrossBasic-IDE/server.cpp"
OUT="server"
RELEASE_DIR="release-64"
ERROR_LOG="error.log"

# --- 1. Compile ---
echo "↪ Compiling ${SRC}..."
# Uses $CXX if set; otherwise falls back to g++
# apt install build-essential libboost-system-dev libboost-thread-dev zlib1g-dev
"${CXX:-g++}" -O3 -march=native -mtune=native -lboost_system -lboost_thread -pthread -lz -o "${OUT}" "${SRC}" 2> "${ERROR_LOG}" \
  || {
    echo "❌ Compilation failed! See ${ERROR_LOG}:"
    cat "${ERROR_LOG}"
    exit 1
  }

# --- 2. Prepare release directory ---
mkdir -p "${RELEASE_DIR}"

# --- 3. Move binary ---
mv -f "${OUT}" "${RELEASE_DIR}/"

# --- 4. Dump shared‑library dependencies ---
echo
echo "🛠 Shared‑library dependencies of ${RELEASE_DIR}/${OUT}:"
if [[ "$(uname)" == "Darwin" ]]; then
  otool -L "${RELEASE_DIR}/${OUT}"
else
  ldd "${RELEASE_DIR}/${OUT}"
fi

# Copy the IDE web folder to the release directory
cp -r ./CrossBasic-IDE/www release-64/

echo
echo "✅ Build complete. Binary is in ${RELEASE_DIR}/"
exit 0
