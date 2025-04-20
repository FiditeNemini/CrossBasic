#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building DateTime.dylib..."
  g++ -shared -fPIC -m64 -o DateTime.dylib DateTime.cpp -pthread
  echo "Build complete: DateTime.dylib"
else
  echo "Detected Linux. Building DateTime.so..."
  g++ -shared -fPIC -m64 -o DateTime.so DateTime.cpp -pthread
  echo "Build complete: DateTime.so"
fi
