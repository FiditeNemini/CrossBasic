#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XAML.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o XAML.dylib XAML.cpp -pthread
  echo "Build complete: XAML.dylib"
else
  echo "Detected Linux. Building XAML.so..."
  g++ -shared -fPIC -m64 -o XAML.so XAML.cpp -pthread
  echo "Build complete: XAML.so"
fi
