# Unitree MuJoCo Workspace

This repository runs Unitree robot controllers against a MuJoCo simulator. The
current workflow is organized around:

- `simulate`: C++ MuJoCo simulator.
- `agent`: controller programs in C++, Python, and ROS2.
- `unitree_robots`: MuJoCo/MJCF robot descriptions and scene files.
- `terrain_tool`: helper for generating terrain scenes.
- `third_party/unitree_sdk2`: vendored Unitree SDK2 used by C++ builds.
- `start.sh`: simple command entrypoint that accepts robot/scene.
- `start.yaml`: tmuxp layout that runs simulator and agent side by side.

The main tested path is the C++ simulator plus the Go2 C++ `main` agent.

## Dependencies

Install common system dependencies:

```bash
sudo apt install libyaml-cpp-dev libspdlog-dev libboost-all-dev libglfw3-dev
```

Install or link MuJoCo:

```bash
cd simulate
ln -s ~/.mujoco/mujoco-3.6.0 mujoco
```

Adjust the MuJoCo path/version if yours is different. `simulate/build.sh`
creates the `mujoco_plugin` link needed by MuJoCo 3.6.x for OBJ/STL mesh
decoders.

For Python agents/simulator:

```bash
pip3 install mujoco pygame numpy pyyaml
```

Install `unitree_sdk2_python` separately if you use the Python path.

## Build

Build the simulator:

```bash
./simulate/build.sh
```

Build the Go2 C++ agent:

```bash
./agent/cpp/go2/build.sh
```

Build the ROS2 agent workspace:

```bash
source ~/unitree_ros2/setup.sh
./agent/ros2/build.sh
```

Python agents do not compile. The Python build script only checks imports:

```bash
./agent/python/build.sh
```

## Run With start.sh And tmuxp

Start the simulator and agent with exactly two runtime values:

```bash
./start.sh robot=go2 scene=scene_terrain.xml
```

This loads `start.yaml`, creating one tmux window with two panes:

- left pane: builds and runs `simulate/build/unitree_mujoco`
- right pane: builds and runs `agent/cpp/<robot>/build/main`

Useful tmux commands:

```text
Ctrl+b then arrow key  switch pane
Ctrl+b then d          detach
tmux attach -t unitree_mujoco
tmux kill-session -t unitree_mujoco
```

## Run Manually

Terminal 1:

```bash
./simulate/build.sh
cd simulate/build
./unitree_mujoco -r go2 -s scene_terrain.xml
```

Terminal 2:

```bash
./agent/cpp/go2/build.sh
cd agent/cpp/go2/build
./main
```

When `main` prints `Press enter to start`, press Enter in the agent pane.

## Configuration

Simulator defaults live in:

```text
simulate/config.yaml
```

Important values:

- `robot`: default robot folder under `unitree_robots`.
- `robot_scene`: default scene file.
- `domain_id`: use `1` for local simulation.
- `interface`: use `lo` for local simulation.
- `use_joystick`: set `0` if no gamepad is attached.

C++/Python agent defaults live in:

```text
agent/config.yaml
```

The default agent DDS settings are:

```yaml
dds:
  domain_id: 1
  interface: "lo"

topics:
  lowcmd: "rt/lowcmd"
  lowstate: "rt/lowstate"
```

## Controller Notes

`agent/cpp/go2/main.cpp` is a simple Go2 demo controller. It publishes `LowCmd` at 500 Hz,
interpolates from a down pose to a stand pose for the first 3 seconds, then
interpolates back down. It is useful for validating the simulator/DDS pipeline,
but it is not a walking or balancing controller.

The controller is Go2-specific because it hard-codes 12 Go2 joint targets.

## Add A C++ Controller

C++ controllers are grouped by robot:

```text
agent/cpp/<robot>/
```

Current robot folders mirror `unitree_robots`: `a2`, `b2`, `b2w`, `g1`,
`go2`, `go2w`, `h1`, `h1_2`, and `r1`.

To add a new controller for a robot, create `main.cpp` in that robot folder.
For example:

```text
agent/cpp/g1/main.cpp
```

Then build that robot's controllers:

```bash
./agent/cpp/g1/build.sh
```

Every robot builds the same executable name:

```text
agent/cpp/<robot>/build/main
```

## Supported Messages

The simulator focuses on low-level sim-to-real development:

- `LowCmd`: motor commands from agent to simulator.
- `LowState`: motor and IMU state from simulator to agent.
- `SportModeState`: virtual base position/velocity.
- `IMUState`: secondary torso IMU for G1-style robots.

Current IDL convention:

- Go-style robots such as Go2, B2, H1, B2w, Go2w use `unitree_go`.
- G1/H1-2 style robots use `unitree_hg`.

## Terrain Tool

See `terrain_tool/readme.md`.

Quick run:

```bash
cd terrain_tool
python3 terrain_generator.py
```

Generated terrain scenes are usually written to
`unitree_robots/<robot>/scene_terrain.xml`.

## Troubleshooting

- `no decoder found for mesh file`: rerun `./simulate/build.sh` and check
  `simulate/build/mujoco_plugin`.
- Agent warning about `lo` multicast: usually harmless for local simulation.
- Robot does not move: make sure simulator and agent use DDS domain `1`,
  interface `lo`, and matching topics.
- Existing tmux session blocks start: run
  `tmux kill-session -t unitree_mujoco`.
