#!/usr/bin/env bash
set -euo pipefail

SRC="XWebView.cpp"
ERROR_LOG="build_error.log"
RELEASE_DIR="release-64"
mkdir -p "${RELEASE_DIR}"

# Determine available webkit package (prefer 4.1 then 4.0)
if pkg-config --exists webkit2gtk-4.1; then
  WEBKIT_PKG="webkit2gtk-4.1"
elif pkg-config --exists webkit2gtk-4.0; then
  WEBKIT_PKG="webkit2gtk-4.0"
else
  echo "❌ Neither webkit2gtk-4.1 nor webkit2gtk-4.0 found. Install libwebkit2gtk dev package." >&2
  exit 1
fi

# Ensure gtk+ is present
if ! pkg-config --exists gtk+-3.0; then
  echo "❌ gtk+-3.0 not found; install libgtk-3-dev." >&2
  exit 1
fi

CFLAGS="$(pkg-config --cflags gtk+-3.0 ${WEBKIT_PKG})"
LIBS="$(pkg-config --libs gtk+-3.0 ${WEBKIT_PKG})"

OUT="XWebView.$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" )"
echo "↪ Building ${OUT} using ${WEBKIT_PKG}..."

g++ -O3 -std=c++17 -fPIC "${SRC}" -shared ${CFLAGS} -o "${OUT}" ${LIBS} -s 2> "${ERROR_LOG}" || {
  echo "❌ Build failed; see ${ERROR_LOG}:" 
  sed -n '1,200p' "${ERROR_LOG}"
  exit 1
}

mv -f "${OUT}" "${RELEASE_DIR}/"
echo "✅ Built ${OUT} -> ${RELEASE_DIR}/"
if [[ "$(uname)" == "Darwin" ]]; then
  otool -L "${RELEASE_DIR}/${OUT}"
else
  ldd "${RELEASE_DIR}/${OUT}" || true
fi
