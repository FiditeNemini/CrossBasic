#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XTextArea.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o XTextArea.dylib XTextArea.cpp -pthread
  echo "Build complete: XTextArea.dylib"
else
  echo "Detected Linux. Building XTextArea.so..."
  g++ -shared -fPIC -m64 -o XTextArea.so XTextArea.cpp -pthread
  echo "Build complete: XTextArea.so"
fi
