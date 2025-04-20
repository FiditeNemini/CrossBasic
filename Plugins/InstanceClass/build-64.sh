#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building InstanceClass.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o InstanceClass.dylib InstanceClass.cpp -pthread
  echo "Build complete: InstanceClass.dylib"
else
  echo "Detected Linux. Building InstanceClass.so..."
  g++ -shared -fPIC -m64 -o InstanceClass.so InstanceClass.cpp -pthread
  echo "Build complete: InstanceClass.so"
fi
