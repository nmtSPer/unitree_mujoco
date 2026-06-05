#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
MUJOCO_LINK="$SCRIPT_DIR/mujoco"
JOBS="${JOBS:-$(nproc)}"

if [ ! -e "$MUJOCO_LINK" ]; then
  LATEST_MUJOCO="$(find "$HOME/.mujoco" -maxdepth 1 -type d -name 'mujoco-*' 2>/dev/null | sort -V | tail -n1 || true)"
  if [ -z "$LATEST_MUJOCO" ]; then
    echo "No MuJoCo installation found in ~/.mujoco and $MUJOCO_LINK does not exist"
    exit 1
  fi
  ln -s "$LATEST_MUJOCO" "$MUJOCO_LINK"
fi

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$BUILD_DIR" -j "$JOBS"

# MuJoCo 3.6.x keeps OBJ/STL decoders in bin/mujoco_plugin. The simulator
# scans for plugins beside the executable, so expose them in the build folder.
if [ -d "$SCRIPT_DIR/mujoco/bin/mujoco_plugin" ] && [ ! -e "$BUILD_DIR/mujoco_plugin" ]; then
  ln -s ../mujoco/bin/mujoco_plugin "$BUILD_DIR/mujoco_plugin"
fi

echo "Built simulator: $BUILD_DIR/unitree_mujoco"
