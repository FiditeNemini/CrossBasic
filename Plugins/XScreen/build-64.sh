#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XScreen.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o XScreen.dylib XScreen.cpp -pthread
  echo "Build complete: XScreen.dylib"
else
  echo "Detected Linux. Building XScreen.so..."
  g++ -shared -fPIC -m64 -o XScreen.so XScreen.cpp -pthread
  echo "Build complete: XScreen.so"
fi
