# Workflows

Commands in this file assume the terminal starts at the repository root:

```bash
cd /path/to/unitree_mujoco
```

## Build The C++ Simulator

```bash
./simulate/build.sh
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
./agent/cpp/go2/build.sh
```

Main output:

- `agent/cpp/go2/build/main`

The C++ agent CMake prefers `third_party/unitree_sdk2` when present, then falls
back to a system install under `/opt/unitree_robotics`.

## Run With start.sh And tmuxp

`start.sh` is the preferred entrypoint for the tmuxp workflow. Run it with
exactly two runtime values:

```bash
./start.sh robot=go2 scene=scene_terrain.xml
```

These values become environment variables consumed by `start.yaml`:

- `UNITREE_ROBOT`: folder name under `unitree_robots`.
- `UNITREE_SCENE`: scene file under `unitree_robots/<robot>`.
- `UNITREE_MUJOCO_ROOT`: repository root resolved by `start.sh`.

`start.sh` loads the repository-local `start.yaml` by absolute path, so it can
also be invoked from another directory:

```bash
/path/to/unitree_mujoco/start.sh robot=go2 scene=scene_terrain.xml
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

The C++ and Python controllers read DDS/topic defaults from
`agent/config.yaml`, which defaults to DDS domain `1`, interface `lo`,
`rt/lowcmd`, and `rt/lowstate`. This matches `simulate/config.yaml` for local
simulation.

CLI override examples:

```bash
./main --domain 1 --interface lo
./main --config ../../../config.yaml
./main eth0
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
./agent/cpp/go2/build.sh
```

If changing the start workflow, verify script parsing:

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
