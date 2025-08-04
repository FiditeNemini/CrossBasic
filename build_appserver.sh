#!/usr/bin/env bash
set -euo pipefail

# --- Configuration ---
SRC="./CrossBasic-IDE/server.cpp"
DEFAULT_OUT="server"
OUT="${1:-$DEFAULT_OUT}"
RELEASE_DIR="release-64"
ERROR_LOG="error.log"
WEB_SRC="./CrossBasic-IDE/www"
WEB_DEST="${RELEASE_DIR}/www"

# --- Helper ---
fail() {
  echo "❌ ERROR: $*" >&2
  exit 1
}

echo "🔧 Build invoked with output binary name: ${OUT}"
# --- Sanity checks ---
if [[ -d "${OUT}" ]]; then
  fail "'${OUT}' exists and is a directory; cannot emit binary with that name. Rename/remove it or supply a different name: ./$(basename "$0") newname"
fi

if [[ ! -f "${SRC}" ]]; then
  fail "Source file '${SRC}' not found."
fi

# Check for Boost headers (allow BOOST_ROOT override)
INCLUDE_FLAGS=""
LIB_FLAGS="-lboost_system -lboost_thread -pthread -lz"
if [[ -n "${BOOST_ROOT:-}" ]]; then
  if [[ ! -d "${BOOST_ROOT}/include/boost" ]]; then
    fail "BOOST_ROOT is set to '${BOOST_ROOT}' but '${BOOST_ROOT}/include/boost' does not exist."
  fi
  INCLUDE_FLAGS="-I${BOOST_ROOT}/include"
  LIB_FLAGS="-L${BOOST_ROOT}/lib -lboost_system -lboost_thread -pthread -lz"
else
  # quick existence check for system boost header
  if ! echo '#include <boost/asio.hpp>' | ${CXX:-g++} -E - >/dev/null 2>&1; then
    echo "[WARN] System Boost headers not found in default include path. If Boost is installed in a custom prefix, set BOOST_ROOT=/path/to/boost and rerun." >&2
  fi
fi

# --- Prepare release directory ---
mkdir -p "${RELEASE_DIR}"

# --- Compile ---
echo "↪ Compiling ${SRC} to ${OUT}..."
if ! command -v "${CXX:-g++}" &>/dev/null; then
  fail "Compiler '${CXX:-g++}' not found in PATH."
fi

# Build invocation: source first, then output, then link flags
"${CXX:-g++}" -O3 -march=native -mtune=native ${INCLUDE_FLAGS} "${SRC}" -o "${OUT}" ${LIB_FLAGS} 2> "${ERROR_LOG}" || {
  echo "❌ Compilation failed! See ${ERROR_LOG}:" 
  sed -n '1,200p' "${ERROR_LOG}" || true
  echo "…(full log at ${ERROR_LOG})"
  exit 1
}

# --- Move binary ---
mv -f "${OUT}" "${RELEASE_DIR}/"

# --- Copy web folder ---
if [[ -d "${WEB_SRC}" ]]; then
  rm -rf "${WEB_DEST}"
  cp -r "${WEB_SRC}" "${WEB_DEST}"
else
  echo "[WARN] Web folder '${WEB_SRC}' does not exist; skipping copy."
fi

# --- Dump shared-library dependencies ---
echo
echo "🛠 Shared-library dependencies of ${RELEASE_DIR}/${OUT}:"
if [[ "$(uname)" == "Darwin" ]]; then
  otool -L "${RELEASE_DIR}/${OUT}"
else
  ldd "${RELEASE_DIR}/${OUT}"
fi

echo
echo "✅ Build complete. Binary is in ${RELEASE_DIR}/"
exit 0
