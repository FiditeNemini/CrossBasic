#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building FolderItem.dylib..."
  g++ -shared -fPIC -m64 -o FolderItem.dylib FolderItem.cpp -pthread
  echo "Build complete: FolderItem.dylib"
else
  echo "Detected Linux. Building FolderItem.so..."
  g++ -shared -fPIC -m64 -o FolderItem.so FolderItem.cpp -pthread
  echo "Build complete: FolderItem.so"
fi
