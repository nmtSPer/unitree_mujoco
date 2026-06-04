#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR"
colcon build

echo "Built ROS2 agent workspace: $SCRIPT_DIR/install"
