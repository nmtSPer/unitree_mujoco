# Architecture Notes

## High-Level Runtime Flow

```text
agent/controller
  -> DDS topic rt/lowcmd
  -> Unitree SDK2 bridge
  -> MuJoCo mj_data.ctrl
  -> MuJoCo physics step
  -> mj_data.sensordata
  -> Unitree SDK2 bridge
  -> DDS topics rt/lowstate, rt/sportmodestate, joystick/IMU topics
  -> agent/controller
```

The simulator acts as a virtual robot. It receives low-level commands from an
agent, applies them to MuJoCo actuators, then publishes state through Unitree
SDK2-style topics so controllers can be reused for sim-to-real work.

## C++ Simulator

Main components:

- `simulate/src/main.cc`
  - Loads MuJoCo, creates the viewer, starts the physics thread and Unitree
    bridge thread.
  - Reads `simulate/config.yaml`; CLI options can override `domain_id`,
    `interface`, `robot`, and `scene`.
  - If `robot_scene` is a relative path, it resolves to
    `unitree_robots/<robot>/<scene>`.
  - Chooses the elastic-band body by preferring `torso_link`, then falling back
    to `base_link`.
  - Keyboard controls: `Backspace` resets the sim; when elastic band is enabled,
    `9` toggles it, `7`/up arrow shortens it, and `8`/down arrow lengthens it.

- `simulate/src/param.h`
  - Holds config for robot, scene, DDS domain/interface, joystick, scene-info
    printing, and elastic band behavior.
  - YAML provides defaults; CLI only overrides options registered in `helper`.

- `simulate/src/unitree_sdk2_bridge.h`
  - `UnitreeSDK2BridgeBase`: finds sensors by name, configures joystick, prints
    scene information.
  - `RobotBridge<LowCmd_t, LowState_t>`:
    - Subscribes to/holds `rt/lowcmd`.
    - Each tick, computes actuator control:
      `tau + kp * (q_des - q) + kd * (dq_des - dq)`.
    - Publishes motor state, IMU, `SportModeState`, and wireless-controller
      data.
  - `Go2Bridge`: uses Unitree go IDL.
  - `G1Bridge`: uses Unitree hg IDL, adds `rt/lf/bmsstate` and
    `rt/secondary_imu`, and sets G1 `mode_machine` based on 23/29 DOF scene
    naming.

Current C++ IDL selection is based on actuator count:

- `m->nu > 20`: create `G1Bridge`/hg-style bridge.
- Otherwise: create `Go2Bridge`/go-style bridge.

This heuristic matters when adding a new high-actuator-count robot.

## Python Simulator

Main components:

- `simulate_python/unitree_mujoco.py`
  - Loads `config.ROBOT_SCENE`, creates the viewer, and starts two threads:
    `SimulationThread` and `PhysicsViewerThread`.
  - `SimulationThread` initializes DDS with `ChannelFactoryInitialize`, creates
    `UnitreeSdk2Bridge`, and steps MuJoCo at `SIMULATE_DT`.

- `simulate_python/unitree_sdk2py_bridge.py`
  - Subscribes to `rt/lowcmd`.
  - Publishes `rt/lowstate`, `rt/sportmodestate`, and `rt/wirelesscontroller`.
  - If `config.ROBOT == "g1"`, imports hg IDL; other robots use go IDL.

The Python bridge currently relies more heavily on `sensordata` ordering, while
the C++ bridge looks sensors up by name. When modifying the Python path, compare
against MJCF and the C++ bridge to avoid index drift.

## Sample Agents

- `agent/cpp/go2/main.cpp`
  - Loads DDS/topic defaults from `agent/config.yaml`.
  - Initializes DDS with `ChannelFactory::Instance()->Init(domain_id,
    interface)`.
  - Publishes the configured low-command topic, normally `rt/lowcmd`, and
    subscribes to the configured low-state topic, normally `rt/lowstate`.
  - Uses a 12-motor Go2 target table and a `tanh` phase to stand up for 3s, then
    stand down.
  - Computes CRC before writing `LowCmd`.

- `agent/python/stand_go2.py`
  - Similar flow using `unitree_sdk2py`.
  - Loads DDS/topic defaults from `agent/config.yaml` and supports CLI
    overrides matching the C++ agent.

- `agent/ros2`
  - ROS2 example that publishes `LowCmd`; CRC helpers live in
    `agent/ros2/include/motor_crc.h` and `agent/ros2/src/motor_crc.cpp`.

## Contracts To Remember

Main DDS topics:

- `rt/lowcmd`: low-level motor command from agent to simulator.
- `rt/lowstate`: motor/IMU state from simulator to agent.
- `rt/sportmodestate`: virtual robot base pose/velocity.
- `rt/secondary_imu`: secondary IMU for G1/hg bridge in C++.
- `rt/lf/bmsstate`: simulated BMS state for G1/hg bridge in C++.

Sensor names expected by the C++ bridge in MJCF:

- Motor sensors in order: q, dq, tau, total size `3 * nu`.
- IMU/body sensors: `imu_quat`, `imu_gyro`, `imu_acc`, `frame_pos`,
  `frame_vel`.
- Secondary IMU: `secondary_imu_quat`, `secondary_imu_gyro`,
  `secondary_imu_acc`.

Special body names:

- `torso_link`: preferred for humanoid elastic-band attachment.
- `base_link`: fallback for quadrupeds or other robots.

## Main Dependencies

- MuJoCo is linked through the `simulate/mujoco` symlink, usually pointing to
  `~/.mujoco/mujoco-<version>`.
- Unitree SDK2 C++ is vendored at `third_party/unitree_sdk2`. `simulate` and
  `agent/cpp/go2` use this copy first, then fall back to
  `/opt/unitree_robotics/lib/cmake` if the vendor directory is absent.
- The C++ simulator needs `yaml-cpp`, `spdlog`, `boost_program_options`, `glfw`,
  `fmt`, `pthread`, `mujoco`, and `unitree_sdk2`.
- The Python simulator needs `unitree_sdk2py`, `mujoco`, and `pygame`.
