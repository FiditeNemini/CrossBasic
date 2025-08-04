#!/usr/bin/env bash
set -euo pipefail

# ---------- configuration ----------
# override with e.g. BOOST_ROOT=/opt/boost ./build_plugins.sh
BOOST_ROOT="${BOOST_ROOT:-/usr/local}"
# assume boost headers in $BOOST_ROOT/include and libs in $BOOST_ROOT/lib
CXX="${CXX:-g++}"
COMMON="-std=c++17 -O3 -s -DNDEBUG -fPIC"
INCL="-I${BOOST_ROOT}/include"
# link boost system/thread statically if available; adjust names if your build uses versioned libs
BOOST_LIBS="-lboost_system -lboost_thread"
OTHER_LIBS="-lz -pthread"
OUT_DIR="release-64/libs"
mkdir -p "${OUT_DIR}"

fail() {
  echo "❌ ERROR: $*" >&2
  exit 1
}

echo
echo "======== 1/4  HttpRequest shared lib ========================"
echo "Compiling HttpRequest..."
"${CXX}" ${COMMON} ${INCL} -shared HttpRequest.cpp -o HttpRequest.$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" ) \
  -Wl,--out-implib,libHttpRequest.a ${BOOST_LIBS} ${OTHER_LIBS} || fail "HttpRequest build failed"

echo
echo "======== 2/4  HttpResponse shared lib ======================="
echo "Compiling HttpResponse..."
"${CXX}" ${COMMON} ${INCL} -shared HttpResponse.cpp -o HttpResponse.$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" ) \
  -Wl,--out-implib,libHttpResponse.a ${BOOST_LIBS} ${OTHER_LIBS} || fail "HttpResponse build failed"

echo
echo "======== 3/4  HttpSession shared lib ========================"
echo "Compiling HttpSession (depends on HttpRequest and HttpResponse)..."
# link against the import libs produced above
"${CXX}" ${COMMON} ${INCL} -shared HttpSession.cpp -o HttpSession.$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" ) \
  libHttpRequest.a libHttpResponse.a ${BOOST_LIBS} ${OTHER_LIBS} -Wl,--out-implib,libHttpSession.a || fail "HttpSession build failed"

echo
echo "======== 4/4  HttpServer shared lib ========================="
echo "Compiling HttpServer (depends on HttpSession and HttpResponse)..."
"${CXX}" ${COMMON} ${INCL} -shared HttpServer.cpp -o HttpServer.$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" ) \
  libHttpSession.a libHttpResponse.a ${BOOST_LIBS} ${OTHER_LIBS} || fail "HttpServer build failed"

# move artifacts
echo
echo "🗂 Moving built plugins into ${OUT_DIR}/"
for lib in HttpRequest HttpResponse HttpSession HttpServer; do
  ext=$( [[ "$(uname)" == "Darwin" ]] && echo "dylib" || echo "so" )
  mv -f "${lib}.${ext}" "${OUT_DIR}/" || true
done
mv -f libHttpRequest.a libHttpResponse.a libHttpSession.a "${OUT_DIR}/" || true

echo
echo "✅ All plugins built successfully. Output in ${OUT_DIR}/"
