#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building CrossbasicFramework.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o CrossbasicFramework.dylib CrossbasicFramework.cpp -pthread
  echo "Build complete: CrossbasicFramework.dylib"
else
  echo "Detected Linux. Building CrossbasicFramework.so..."
  g++ -shared -fPIC -m64 -o CrossbasicFramework.so CrossbasicFramework.cpp -pthread
  echo "Build complete: CrossbasicFramework.so"
fi
