#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building MemoryBlock.dylib..."
  g++ -shared -fPIC -m64 -o MemoryBlock.dylib MemoryBlock.cpp -pthread
  echo "Build complete: MemoryBlock.dylib"
else
  echo "Detected Linux. Building MemoryBlock.so..."
  g++ -shared -fPIC -m64 -o MemoryBlock.so MemoryBlock.cpp -pthread
  echo "Build complete: MemoryBlock.so"
fi
