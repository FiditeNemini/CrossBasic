#!/usr/bin/env bash
if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building SimCipher.dylib..."
  g++ -shared -fPIC -m64 -o SimCipher.dylib SimCipher.cpp -pthread
  echo "Build complete: SimCipher.dylib"
else
  echo "Detected Linux. Building SimCipher.so..."
  g++ -shared -fPIC -m64 -o SimCipher.so SimCipher.cpp -pthread
  echo "Build complete: SimCipher.so"
fi