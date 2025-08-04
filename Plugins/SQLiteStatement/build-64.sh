#!/usr/bin/env bash
set -euo pipefail

SRC="SQLiteStatement.cpp"
DB_LIB="./SQLiteDatabase.so"   # on mac use ./SQLiteDatabase.dylib if building there

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building SQLiteStatement.dylib..."
  g++ -s -shared -fPIC -m64 -O3 -o SQLiteStatement.dylib "${SRC}" "${DB_LIB%.*}.dylib" -lsqlite3 \
      -Wl,-rpath,@loader_path
  echo "Build complete: SQLiteStatement.dylib"
else
  echo "Detected Linux. Building SQLiteStatement.so..."
  g++ -s -shared -fPIC -m64 -O3 -o SQLiteStatement.so "${SRC}" "${DB_LIB}" -lsqlite3 \
      -Wl,-rpath='$ORIGIN'
  echo "Build complete: SQLiteStatement.so"
fi
