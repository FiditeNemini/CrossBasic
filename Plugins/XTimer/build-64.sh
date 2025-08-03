#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XTimer.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o XTimer.dylib XTimer.cpp -pthread
  echo "Build complete: XTimer.dylib"
else
  echo "Detected Linux. Building XTimer.so..."
  g++ -shared -fPIC -m64 -o XTimer.so XTimer.cpp -pthread
  echo "Build complete: XTimer.so"
fi
