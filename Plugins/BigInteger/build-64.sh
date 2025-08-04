#!/usr/bin/env bash
set -euo pipefail

SRC="BigInteger.cpp"
ERROR_LOG="build_error.log"

fail() {
  echo "❌ ERROR: $*" >&2
  exit 1
}

echo "🔧 Building BigInteger shared library from ${SRC} on $(uname)..."

if [[ ! -f "${SRC}" ]]; then
  fail "Source '${SRC}' not found."
fi
if ! command -v g++ &>/dev/null; then
  fail "g++ not found in PATH."
fi

# Ensure pkg-config and GMP are available
platform="$(uname)"
if ! command -v pkg-config &>/dev/null; then
  echo "[INFO] pkg-config missing; attempting to install prerequisites..."
  if [[ "$platform" == "Darwin" ]]; then
    if command -v brew &>/dev/null; then
      brew install pkg-config gmp
    else
      fail "Homebrew not detected; install Homebrew and then run: brew install pkg-config gmp"
    fi
  else
    if command -v apt &>/dev/null; then
      sudo apt update
      sudo apt install -y pkg-config libgmp-dev build-essential
    elif command -v dnf &>/dev/null; then
      sudo dnf install -y pkgconf-pkg-config gmp-devel make gcc-c++
    else
      echo "[WARN] Cannot auto-install pkg-config/GMP on this distro; please install them manually."
    fi
  fi
fi

# Verify gmp is visible
if ! pkg-config --exists gmp; then
  echo "[INFO] pkg-config does not yet see gmp; attempting to install (if not already)..."
  if [[ "$platform" == "Darwin" ]]; then
    brew install gmp || true
  else
    if command -v apt &>/dev/null; then
      sudo apt install -y libgmp-dev || true
    elif command -v dnf &>/dev/null; then
      sudo dnf install -y gmp-devel || true
    fi
  fi
fi

if ! pkg-config --exists gmp; then
  fail "pkg-config still cannot find gmp. Ensure GMP development headers are installed and PKG_CONFIG_PATH is correct."
fi

CFLAGS="$(pkg-config --cflags gmp)"
LIBS="$(pkg-config --libs gmp)"

# Build for platform
if [[ "$platform" == "Darwin" ]]; then
  OUT="libBigInteger.dylib"
  echo "↪ Compiling macOS dynamic library ${OUT}..."
  # -dynamiclib to produce .dylib; install_name set to @rpath for relocatability
  echo "g++ -dynamiclib ${CFLAGS} \"${SRC}\" -o \"${OUT}\" ${LIBS} -O3 -std=c++17 -s -install_name @rpath/${OUT}"
  g++ -dynamiclib ${CFLAGS} "${SRC}" -o "${OUT}" ${LIBS} -O3 -std=c++17 -s -install_name @rpath/"${OUT}" 2> "${ERROR_LOG}" || {
    echo "❌ Build failed; see ${ERROR_LOG}:" 
    sed -n '1,200p' "${ERROR_LOG}" || true
    fail "dylib build failed."
  }
  echo "✅ Built ${OUT}"
  echo "Dependencies:"
  otool -L "${OUT}"
else
  OUT="libBigInteger.so"
  echo "↪ Compiling Linux shared object ${OUT}..."
  echo "g++ -shared -fPIC ${CFLAGS} \"${SRC}\" -o \"${OUT}\" ${LIBS} -O3 -std=c++17 -s"
  g++ -shared -fPIC ${CFLAGS} "${SRC}" -o "${OUT}" ${LIBS} -O3 -std=c++17 -s 2> "${ERROR_LOG}" || {
    echo "❌ Build failed; see ${ERROR_LOG}:"
    sed -n '1,200p' "${ERROR_LOG}" || true
    fail "so build failed."
  }
  echo "✅ Built ${OUT}"
  echo "Dependencies:"
  ldd "${OUT}" || true
fi
