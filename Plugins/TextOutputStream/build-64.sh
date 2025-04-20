#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building TextOutputStream.dylib..."
  g++ -shared -fPIC -m64 -o TextOutputStream.dylib TextOutputStream.cpp -pthread
  echo "Build complete: TextOutputStream.dylib"
else
  echo "Detected Linux. Building TextOutputStream.so..."
  g++ -shared -fPIC -m64 -o TextOutputStream.so TextOutputStream.cpp -pthread
  echo "Build complete: TextOutputStream.so"
fi
