#!/usr/bin/env bash
set -euo pipefail

# --- Configuration ---
SRC="xcompile.cpp"
OUT="xcompile"
RELEASE_DIR="release-64"
ERROR_LOG="error.log"

# --- 1. Compile ---
echo "↪ Compiling ${SRC}..."
# Uses $CXX if set; otherwise falls back to g++
"${CXX:-g++}" -O3 -march=native -mtune=native -o "${OUT}" "${SRC}" 2> "${ERROR_LOG}" \
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

echo
echo "✅ Build complete. Binary is in ${RELEASE_DIR}/"
exit 0
