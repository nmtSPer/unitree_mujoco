#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
JOBS="${JOBS:-$(nproc)}"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$SCRIPT_DIR"
make -j"$JOBS"

if [ -x "$BUILD_DIR/main" ]; then
  echo "Built C++ agent: $BUILD_DIR/main"
else
  echo "No main.cpp controller found for: $SCRIPT_DIR"
fi
