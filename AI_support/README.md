# AI Support Knowledge Base

This directory is the starting point for AI agents or new developers working in
the `unitree_mujoco` repository. Its purpose is to provide a fast internal map:
what this repo contains, where the main execution flow lives, how to build/run
it, and what to watch out for when making changes.

Created: 2026-06-04.

## Recommended Reading Order

1. `README.md`: quick repository map.
2. `ARCHITECTURE.md`: data flow between MuJoCo, Unitree SDK2 DDS, and agents.
3. `WORKFLOWS.md`: build, run, configuration, and terrain workflows.
4. `CHANGE_GUIDE.md`: checklists for changing/extending the simulator, agents,
   and robot MJCF files.

## Repository Summary

This repository is a simulator built on MuJoCo and Unitree SDK2. It contains two
simulator implementations:

- `simulate`: main C++ simulator, recommended by the root README.
- `simulate_python`: Python simulator for quick tests or prototypes.
- `unitree_robots`: MJCF files, meshes, scenes, and terrain assets for Unitree
  robots.
- `agent`: controllers in C++, Python, and ROS2.
- `terrain_tool`: terrain generation utilities using primitives and heightmaps.
- `third_party/unitree_sdk2`: clean vendored Unitree SDK2 copy used by both the
  simulator and C++ agents.

Current robot folders under `unitree_robots`: `a2`, `b2`, `b2w`, `g1`, `go2`,
`go2w`, `h1`, `h1_2`, `r1`.

## Important Entry Points

- `start.sh`: command entrypoint accepting `robot=... scene=...`, exporting
  environment variables, then calling tmuxp.
- `start.yaml`: runs the C++ simulator and C++ agent side by side in tmux.
- `third_party/unitree_sdk2`: vendored SDK used by `simulate` and `agent/cpp/go2`
  CMake before falling back to a system install.
- `simulate/build.sh`: builds the C++ simulator into `simulate/build`.
- `simulate/src/main.cc`: MuJoCo main loop, config loading, bridge thread setup.
- `simulate/src/param.h`: reads `simulate/config.yaml` and parses CLI options.
- `simulate/src/unitree_sdk2_bridge.h`: bridge between MuJoCo data and DDS
  topics.
- `agent/config.yaml`: shared DDS/topic defaults for controllers.
- `agent/cpp/go2/main.cpp`: sample Go2 controller publishing `rt/lowcmd`.

## Useful Defaults For AI Agents

- Prefer the C++ simulator in `simulate` for general simulator work, unless the
  task explicitly targets Python or ROS2.
- For local simulation, use DDS domain `1` and interface `lo` to avoid the real
  robot's default domain.
- When touching `LowCmd`, `LowState`, `SportModeState`, or IMU behavior, inspect
  `simulate/src/unitree_sdk2_bridge.h` first.
- For robot model work, start with `unitree_robots/<robot>/scene*.xml` and the
  main robot XML, such as `go2.xml` or `g1_29dof.xml`.
- Do not assume every robot uses the same IDL: Go-style robots use
  `unitree_go`; G1/H1-2 and high-actuator-count robots use `unitree_hg` in the
  current C++ simulator.

## Areas To Treat Carefully

- `third_party/unitree_sdk2/` is a vendored dependency tree. Modify it only when
  the task clearly requires an SDK update.
- `/unitree_sdk2/` may exist locally as a nested SDK clone, but it is ignored by
  this repo to avoid embedded Git repository warnings.
- `simulate/build/`, `agent/cpp/go2/build/`, and generated binaries are build
  outputs, not source.
- The working tree may already contain user changes. Always check
  `git status --short` before editing, and never revert changes you did not
  make.
