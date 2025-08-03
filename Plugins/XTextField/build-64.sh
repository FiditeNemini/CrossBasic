#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XTextField.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o XTextField.dylib XTextField.cpp -pthread
  echo "Build complete: XTextField.dylib"
else
  echo "Detected Linux. Building XTextField.so..."
  g++ -shared -fPIC -m64 -o XTextField.so XTextField.cpp -pthread
  echo "Build complete: XTextField.so"
fi
