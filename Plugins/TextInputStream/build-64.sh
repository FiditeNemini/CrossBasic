#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building TextInputStream.dylib..."
  g++ -shared -fPIC -m64 -o TextInputStream.dylib TextInputStream.cpp -pthread
  echo "Build complete: TextInputStream.dylib"
else
  echo "Detected Linux. Building TextInputStream.so..."
  g++ -shared -fPIC -m64 -o TextInputStream.so TextInputStream.cpp -pthread
  echo "Build complete: TextInputStream.so"
fi
