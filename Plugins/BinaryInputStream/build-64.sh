#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building BinaryInputStream.dylib..."
  g++ -shared -fPIC -m64 -o BinaryInputStream.dylib BinaryInputStream.cpp -pthread
  echo "Build complete: BinaryInputStream.dylib"
else
  echo "Detected Linux. Building BinaryInputStream.so..."
  g++ -shared -fPIC -m64 -o BinaryInputStream.so BinaryInputStream.cpp -pthread
  echo "Build complete: BinaryInputStream.so"
fi
