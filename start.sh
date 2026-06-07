#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNITREE_ROBOT="${UNITREE_ROBOT:-}"
UNITREE_SCENE="${UNITREE_SCENE:-}"
UNITREE_MUJOCO_ROOT="$ROOT_DIR"

usage() {
  cat <<EOF
Usage:
  ./start.sh robot=go2 scene=scene_terrain.xml

Required parameters:
  robot=NAME        Robot folder name, for example: go2
  scene=FILE        Scene file, for example: scene_terrain.xml
EOF
}

if [ "$#" -eq 1 ] && { [ "$1" = "-h" ] || [ "$1" = "--help" ]; }; then
  usage
  exit 0
fi

if [ "$#" -ne 2 ]; then
  echo "Expected exactly 2 parameters." >&2
  usage >&2
  exit 1
fi

for arg in "$@"; do
  case "$arg" in
    robot=*) UNITREE_ROBOT="${arg#robot=}" ;;
    scene=*) UNITREE_SCENE="${arg#scene=}" ;;
    *)
      echo "Unknown parameter: $arg" >&2
      usage >&2
      exit 1
      ;;
  esac
done

: "${UNITREE_ROBOT:?Missing robot=...}"
: "${UNITREE_SCENE:?Missing scene=...}"

if ! command -v tmuxp >/dev/null 2>&1; then
  echo "tmuxp is required for start.sh. Install tmuxp or run the simulator and agent manually." >&2
  exit 1
fi

if [ ! -x "$ROOT_DIR/simulate/build.sh" ]; then
  echo "Simulator build script not found: $ROOT_DIR/simulate/build.sh" >&2
  exit 1
fi

if [ ! -x "$ROOT_DIR/agent/cpp/$UNITREE_ROBOT/build.sh" ]; then
  echo "Agent build script not found: $ROOT_DIR/agent/cpp/$UNITREE_ROBOT/build.sh" >&2
  exit 1
fi

if [ ! -f "$ROOT_DIR/unitree_robots/$UNITREE_ROBOT/$UNITREE_SCENE" ]; then
  echo "Scene file not found: $ROOT_DIR/unitree_robots/$UNITREE_ROBOT/$UNITREE_SCENE" >&2
  exit 1
fi

export UNITREE_ROBOT
export UNITREE_SCENE
export UNITREE_MUJOCO_ROOT

cd "$ROOT_DIR"
tmuxp load "$ROOT_DIR/start.yaml"
