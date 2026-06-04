# Change Guide For AI Agents And Developers

## Working Principles

- Run `git status --short` before editing. This repo may already contain user
  changes.
- Keep changes scoped to the task. Do not revert files you did not modify.
- Prefer the C++ simulator for general simulator work.
- Do not modify `third_party/unitree_sdk2` unless the task clearly requires a
  dependency change.
- Do not commit the local `/unitree_sdk2` clone. It is ignored; the clean vendor
  copy lives at `third_party/unitree_sdk2`.
- Do not commit build artifacts from `simulate/build` or `agent/cpp/*/build`.

## When Changing The DDS/MuJoCo Bridge

Read first:

- `simulate/src/unitree_sdk2_bridge.h`
- `simulate/src/main.cc`
- `simulate/src/param.h`
- The relevant robot MJCF under `unitree_robots/<robot>/`

Checklist:

- Are the topics still `rt/lowcmd`, `rt/lowstate`, and `rt/sportmodestate`?
- Do the simulator and agent use the same DDS domain and interface?
- Does the robot use the correct IDL: `unitree_go` or `unitree_hg`?
- Does `mj_model->nu` match the number of motors controlled by the agent?
- Do the MJCF sensor names include `imu_quat`, `imu_gyro`, `imu_acc`,
  `frame_pos`, and `frame_vel` when the code needs them?
- For humanoid/G1 work, is there any need to handle `secondary_imu_*`,
  `rt/secondary_imu`, `rt/lf/bmsstate`, or `mode_machine`?

## When Adding A New Robot

1. Create `unitree_robots/<robot>/` with the robot XML, `scene.xml`, and
   mesh/assets.
2. Make sure the scene includes the robot XML and contains the sensor/body names
   expected by the bridge.
3. Run the simulator:

```bash
cd simulate/build
./unitree_mujoco -r <robot> -s scene.xml
```

4. If actuator count is greater than 20, the current C++ simulator selects the
   hg/G1 bridge. If the new robot is not G1 but has many actuators, revisit the
   selection logic.
5. Update the controller/agent if motor ordering differs.

## When Adding A New C++ Agent

1. Add the source file under `agent/cpp/go2`.
2. Update `agent/cpp/go2/CMakeLists.txt` to build the new executable.
3. Prefer reading DDS/topic settings from `agent/config.yaml` instead of
   hardcoding domain, interface, or topic names.
4. Initialize DDS with domain `1` and interface `lo` for local simulation.
5. Publish `rt/lowcmd` and subscribe to `rt/lowstate`, unless the config
   intentionally overrides those topics.
6. For `LowCmd`, compute CRC before writing.
7. Test with:

```bash
./start.sh robot=<robot> scene=<scene.xml>
```

## When Changing The Start Workflow

Keep this contract intact:

- `start.sh` accepts exactly two parameters: `robot=... scene=...`.
- `start.yaml` uses `UNITREE_ROBOT` and `UNITREE_SCENE`.
- The simulator pane builds/runs the simulator; the agent pane builds/runs the
  selected robot's `main` executable.

After editing, run:

```bash
./start.sh --help
```

If `tmuxp` is available, run one full command as a smoke test.

## When Changing Terrain

Read:

- `terrain_tool/readme.md`
- `terrain_tool/terrain_generator.py`
- The output scene under `unitree_robots/<robot>/scene_terrain.xml`

Watch for:

- Output paths are relative to `terrain_tool`.
- Heightmap images should land in the correct robot folder.
- Terrain scenes must load the robot XML and assets through valid paths.

## When Changing The Python Simulator

Read:

- `simulate_python/unitree_mujoco.py`
- `simulate_python/unitree_sdk2py_bridge.py`
- `simulate_python/config.py`

Watch for:

- The Python bridge currently switches to hg IDL only when
  `config.ROBOT == "g1"`.
- Several Python sensor indices depend on `sensordata` ordering. If MJCF sensor
  order changes, update the code or move toward name-based lookup like the C++
  bridge.
- Disable `USE_JOYSTICK` when no gamepad is attached.

## When Changing Sample Agent Configuration

Read:

- `agent/config.yaml`
- `agent/cpp/go2/main.cpp`
- `agent/python/stand_go2.py`

Keep C++ and Python agent CLI behavior aligned:

- `--config PATH` selects a YAML config.
- `--domain ID` overrides `dds.domain_id`.
- `--interface IFACE` overrides `dds.interface`.
- A single positional interface argument is a legacy shortcut for domain `0`.

## Suggested Validation By Change Type

- C++ build/system: `./simulate/build.sh`, `./agent/cpp/go2/build.sh`.
- CLI/start: `./start.sh --help`, then a Go2 smoke test.
- Bridge/topic: run simulator and sample agent; verify the agent receives
  `LowState` and the robot reacts to `LowCmd`.
- MJCF/sensor: enable `print_scene_information: 1` and inspect printed names and
  indices.
- Python: `python3 simulate_python/unitree_mujoco.py` and
  `python3 simulate_python/test/test_unitree_sdk2.py`.
