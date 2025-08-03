#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XThread.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o XThread.dylib XThread.cpp -pthread
  echo "Build complete: XThread.dylib"
else
  echo "Detected Linux. Building XThread.so..."
  g++ -shared -fPIC -m64 -o XThread.so XThread.cpp -pthread
  echo "Build complete: XThread.so"
fi
