#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building SQLiteStatement.dylib..."
  g++ -s -shared -fPIC -m64 -O3 -o SQLiteStatement.dylib SQLiteStatement.cpp -lsqlite3 -L. -lSQLiteDatabase
  echo "Build complete: SQLiteStatement.dylib"
else
  echo "Detected Linux. Building SQLiteStatement.so..."
  g++ -s -shared -fPIC -m64 -O3 -o SQLiteStatement.so SQLiteStatement.cpp -lsqlite3 -L. -lSQLiteDatabase
  echo "Build complete: SQLiteStatement.so"
fi
