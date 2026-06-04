#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
JOBS="${JOBS:-$(nproc)}"

if [ -f "$BUILD_DIR/CMakeCache.txt" ] && ! grep -q "CMAKE_HOME_DIRECTORY:INTERNAL=$SCRIPT_DIR" "$BUILD_DIR/CMakeCache.txt"; then
  echo "Detected stale CMake cache in $BUILD_DIR; recreating build directory"
  cmake -E rm -rf "$BUILD_DIR"
fi

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j "$JOBS"

if [ -x "$BUILD_DIR/main" ]; then
  echo "Built C++ agent: $BUILD_DIR/main"
else
  echo "No main.cpp controller found for: $SCRIPT_DIR"
fi
