#!/usr/bin/env bash
set -euo pipefail

# --- Config ---
SRC="./CrossBasic-SRC/crossbasic.cpp"
BIN_NAME="crossbasic"
RELEASE_DIR="release-64"
ERROR_LOG="error.log"
ERROR_LIB_LOG="errorlib.log"
SCRIPTS_SRC="Scripts"
RUNALL_SCRIPT="runallscripts.sh"

# --- Helper ---
fail() {
  echo "❌ ERROR: $*" >&2
  exit 1
}

echo "🔧 Building CrossBasic..."

# Sanity checks
if [[ ! -f "${SRC}" ]]; then
  fail "Source file '${SRC}' not found."
fi

if ! command -v g++ &>/dev/null; then
  fail "g++ compiler not found in PATH."
fi

# Ensure release directory
mkdir -p "${RELEASE_DIR}"

# --- 1. Build executable ---
echo "↪ Compiling executable ${BIN_NAME}..."
# Prevent collisions if directory with same name exists
if [[ -d "${BIN_NAME}" ]]; then
  fail "'${BIN_NAME}' exists and is a directory; cannot emit binary with that name."
fi

g++ -O3 -march=native -mtune=native -flto -m64 "${SRC}" -o "${BIN_NAME}" -lffi 2> "${ERROR_LOG}" || {
  echo "❌ Compilation of executable failed. See ${ERROR_LOG}:" 
  sed -n '1,200p' "${ERROR_LOG}" || true
  fail "Executable build failed."
}

# Move executable
mv -f "${BIN_NAME}" "${RELEASE_DIR}/"

# --- 2. Build embeddable/shared library variant ---
if [[ "$(uname)" == "Darwin" ]]; then
  LIB_OUT="crossbasic.dylib"
  echo "↪ Building macOS dynamic library ${LIB_OUT}..."
  # Position-independent code and dynamic lib
  g++ -dynamiclib -O3 -march=native -mtune=native -flto -m64 "${SRC}" -o "${LIB_OUT}" -lffi 2> "${ERROR_LIB_LOG}" || {
    echo "❌ Dynamic library build failed. See ${ERROR_LIB_LOG}:"
    sed -n '1,200p' "${ERROR_LIB_LOG}" || true
    fail "dylib build failed."
  }
  mv -f "${LIB_OUT}" "${RELEASE_DIR}/"
  echo "Dylib dependencies:"
  otool -L "${RELEASE_DIR}/${LIB_OUT}"
elif [[ "$(uname)" == "Linux" ]]; then
  LIB_OUT="crossbasic.so"
  echo "↪ Building Linux shared object ${LIB_OUT}..."
  g++ -shared -fPIC -O3 -march=native -mtune=native -flto -m64 "${SRC}" -o "${LIB_OUT}" -lffi 2> "${ERROR_LIB_LOG}" || {
    echo "❌ Shared object build failed. See ${ERROR_LIB_LOG}:"
    sed -n '1,200p' "${ERROR_LIB_LOG}" || true
    fail "so build failed."
  }
  mv -f "${LIB_OUT}" "${RELEASE_DIR}/"
  echo "Shared library dependencies of executable:"
  ldd "${RELEASE_DIR}/${BIN_NAME}"
else
  echo "⚠️ Unsupported OS: $(uname); skipping shared library build."
fi

# --- 3. Copy auxiliary resources ---
if [[ -d "${SCRIPTS_SRC}" ]]; then
  rm -rf "${RELEASE_DIR}/${SCRIPTS_SRC}"
  cp -r "${SCRIPTS_SRC}" "${RELEASE_DIR}/"
else
  echo "[WARN] Scripts directory '${SCRIPTS_SRC}' not found; skipping."
fi

if [[ -f "${RUNALL_SCRIPT}" ]]; then
  cp -f "${RUNALL_SCRIPT}" "${RELEASE_DIR}/"
else
  echo "[WARN] Run-all script '${RUNALL_SCRIPT}' not found; skipping."
fi

echo
echo "✅ CrossBasic Built Successfully. Contents of ${RELEASE_DIR}/:"
ls -lah "${RELEASE_DIR}/"
exit 0
