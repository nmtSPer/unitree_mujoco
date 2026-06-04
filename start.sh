#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTROLLER=""
ROBOT=""
SCENE=""

usage() {
  cat <<EOF
Usage:
  ./start.sh controller=stand_go2 robot=go2 scene=scene_terrain.xml

Required parameters:
  controller=NAME         Agent executable name, for example: stand_go2
  robot=NAME              Robot name, for example: go2, b2, g1, h1
  scene=FILE              Scene file, for example: scene.xml, scene_terrain.xml

Examples:
  ./start.sh controller=stand_go2 robot=go2 scene=scene_terrain.xml
  ./start.sh controller=stand_go2 robot=b2 scene=scene.xml
EOF
}

if [ "$#" -eq 1 ] && { [ "$1" = "-h" ] || [ "$1" = "--help" ]; }; then
  usage
  exit 0
fi

if [ "$#" -ne 3 ]; then
  echo "Expected exactly 3 parameters: controller=..., robot=..., scene=..." >&2
  usage >&2
  exit 1
fi

while [ "$#" -gt 0 ]; do
  case "$1" in
    controller=*)
      CONTROLLER="${1#controller=}"
      shift
      ;;
    robot=*)
      ROBOT="${1#robot=}"
      shift
      ;;
    scene=*)
      SCENE="${1#scene=}"
      shift
      ;;
    *)
      echo "Unknown parameter: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [ -z "$CONTROLLER" ] || [ -z "$ROBOT" ] || [ -z "$SCENE" ]; then
  echo "controller, robot, and scene must not be empty" >&2
  usage >&2
  exit 1
fi

case "$CONTROLLER" in
  */*|*..*)
    echo "controller must be an executable name, not a path: $CONTROLLER" >&2
    exit 1
    ;;
esac

export UNITREE_CONTROLLER="$CONTROLLER"
export UNITREE_ROBOT="$ROBOT"
export UNITREE_SCENE="$SCENE"

cd "$ROOT_DIR"
tmuxp load start.yaml
