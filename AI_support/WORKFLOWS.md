# Workflows

## Build The C++ Simulator

```bash
cd simulate
./build.sh
```

Main outputs:

- `simulate/build/unitree_mujoco`
- `simulate/build/jstest`

`simulate/build.sh` also creates `simulate/build/mujoco_plugin` when MuJoCo
3.6.x keeps OBJ/STL decoder plugins under `simulate/mujoco/bin/mujoco_plugin`.

The simulator CMake prefers `third_party/unitree_sdk2` when present, then falls
back to a system install under `/opt/unitree_robotics`.

## Build The Sample C++ Agent

```bash
cd agent/cpp
./build.sh
```

Main output:

- `agent/cpp/build/stand_go2`

The C++ agent CMake prefers `third_party/unitree_sdk2` when present, then falls
back to a system install under `/opt/unitree_robotics`.

## Run With start.sh

`start.sh` uses `tmuxp` to run the simulator and controller in two panes.

```bash
./start.sh controller=stand_go2 robot=go2 scene=scene_terrain.xml
```

Always provide all three parameters:

- `controller`: executable name under `agent/cpp/build`, not a path.
- `robot`: folder name under `unitree_robots`.
- `scene`: scene file under `unitree_robots/<robot>`.

Exported environment variables:

- `UNITREE_CONTROLLER`
- `UNITREE_ROBOT`
- `UNITREE_SCENE`

## Run Manually

Terminal 1:

```bash
cd simulate
./build.sh
cd build
./unitree_mujoco -r go2 -s scene_terrain.xml
```

Terminal 2:

```bash
cd agent/cpp
./build.sh
cd build
./stand_go2
```

The C++ and Python sample agents read DDS/topic defaults from
`agent/config.yaml`, which defaults to DDS domain `1`, interface `lo`,
`rt/lowcmd`, and `rt/lowstate`. This matches `simulate/config.yaml` for local
simulation.

CLI override examples:

```bash
./stand_go2 --domain 1 --interface lo
./stand_go2 --config ../../config.yaml
./stand_go2 eth0
```

The final form is kept for legacy real-robot runs: it uses domain `0` and the
given interface.

## Configure The C++ Agent

File: `agent/config.yaml`.

```yaml
dds:
  domain_id: 1
  interface: "lo"

topics:
  lowcmd: "rt/lowcmd"
  lowstate: "rt/lowstate"
```

Use domain `1`/`lo` for local MuJoCo simulation. For a real robot, use the
robot's expected domain, commonly `0`, and the physical network interface.

## Configure The C++ Simulator

File: `simulate/config.yaml`.

Important options:

- `robot`: robot folder, for example `go2`, `b2`, `g1`, `h1`.
- `robot_scene`: scene XML, for example `scene.xml`, `scene_terrain.xml`.
- `domain_id`: DDS domain. Simulation should usually use `1`; real robots
  commonly default to `0`.
- `interface`: network interface. Local simulation should use `lo`.
- `use_joystick`: set to `0` when no gamepad is attached.
- `joystick_type`: `xbox` or `switch`.
- `print_scene_information`: set to `1` to print link/joint/actuator/sensor
  indices when debugging MJCF.
- `enable_elastic_band`: set to `1` for humanoids that need a virtual lifting
  strap during initialization.

CLI override example:

```bash
./unitree_mujoco -r go2 -s scene_terrain.xml -i 1 -n lo
```

## Python Simulator

Config: `simulate_python/config.py`.

Run simulator:

```bash
cd simulate_python
python3 unitree_mujoco.py
```

Run test publisher/subscriber:

```bash
cd simulate_python
python3 test/test_unitree_sdk2.py
```

When changing robot, update `ROBOT` and `ROBOT_SCENE`. The current Python bridge
switches to hg IDL only when `ROBOT == "g1"`.

Run Python agent:

```bash
cd agent/python
python3 stand_go2.py
python3 stand_go2.py --domain 1 --interface lo
python3 stand_go2.py eth0
```

The final form is the legacy shortcut for domain `0` on the given interface.

## Terrain Tool

```bash
cd terrain_tool
python3 terrain_generator.py
```

The tool creates or updates terrain scenes, commonly writing to
`unitree_robots/<robot>/scene_terrain.xml` plus the matching heightmap image.

When using a terrain scene:

- C++: set `robot_scene: "scene_terrain.xml"` or run with
  `-s scene_terrain.xml`.
- Python: set
  `ROBOT_SCENE = "../unitree_robots/" + ROBOT + "/scene_terrain.xml"`.

## Quick Validation After Changes

Recommended minimum:

```bash
./simulate/build.sh
./agent/cpp/build.sh
```

If changing the start workflow:

```bash
./start.sh --help
```

If changing bridge/MJCF/config behavior, run the simulator with the affected
robot and enable `print_scene_information: 1` to inspect sensor/body names.

## Common Failure Modes

- MuJoCo not found: check the `simulate/mujoco` symlink.
- `unitree_sdk2` not found: check `third_party/unitree_sdk2`, or the fallback
  install under `/opt/unitree_robotics` / `CMAKE_PREFIX_PATH`.
- Agent does not receive state: check `domain_id`, network interface, topic
  names, and IDL selection.
- Robot does not move: check `LowCmd` CRC, motor mode, kp/kd/tau, and whether
  the agent motor count matches the MJCF actuator count.
- Mesh/OBJ/STL does not load on newer MuJoCo: check the `mujoco_plugin` symlink
  in the build directory.
