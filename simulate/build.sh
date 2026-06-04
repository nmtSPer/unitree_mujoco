#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
JOBS="${JOBS:-$(nproc)}"

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j "$JOBS"

# MuJoCo 3.6.x keeps OBJ/STL decoders in bin/mujoco_plugin. The simulator
# scans for plugins beside the executable, so expose them in the build folder.
if [ -d "$SCRIPT_DIR/mujoco/bin/mujoco_plugin" ] && [ ! -e "$BUILD_DIR/mujoco_plugin" ]; then
  ln -s ../mujoco/bin/mujoco_plugin "$BUILD_DIR/mujoco_plugin"
fi

echo "Built simulator: $BUILD_DIR/unitree_mujoco"
