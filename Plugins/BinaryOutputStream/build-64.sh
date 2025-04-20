#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building BinaryOutputStream.dylib..."
  g++ -shared -fPIC -m64 -o BinaryOutputStream.dylib BinaryOutputStream.cpp -pthread
  echo "Build complete: BinaryOutputStream.dylib"
else
  echo "Detected Linux. Building BinaryOutputStream.so..."
  g++ -shared -fPIC -m64 -o BinaryOutputStream.so BinaryOutputStream.cpp -pthread
  echo "Build complete: BinaryOutputStream.so"
fi
