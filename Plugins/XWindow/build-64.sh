#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XWindow.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o XWindow.dylib XWindow.cpp -pthread
  echo "Build complete: XWindow.dylib"
else
  echo "Detected Linux. Building XWindow.so..."
  g++ -shared -fPIC -m64 -o XWindow.so XWindow.cpp -pthread
  echo "Build complete: XWindow.so"
fi
