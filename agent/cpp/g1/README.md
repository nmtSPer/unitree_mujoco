# G1 C++ Controllers

Minimal G1 low-level controller scaffold for joint-domain RL.

Commands below assume the terminal is at the repository root:

```bash
cd /path/to/unitree_mujoco
```

Build:

```bash
./agent/cpp/g1/build.sh
```

Run:

```bash
./agent/cpp/g1/build/main --config ./agent/cpp/g1/config.yaml
```

Validate config without starting DDS:

```bash
./agent/cpp/g1/build/main --config ./agent/cpp/g1/config.yaml --check-config
```

Plot joint references and measured positions in PlotJuggler:

```text
Streaming -> UDP Server -> JSON -> port 9870
```

The G1 agent publishes flattened JSON keys named `q_ref/<joint_index>` and
`q/<joint_index>` when `telemetry.enabled` is true in `config/common.yaml`.
For example, plot `q_ref/0` and `q/0` together to compare the reference and
current position for joint 0.

Run with the simulator tmux layout:

```bash
./start.sh robot=g1 scene=scene.xml
```

Keyboard:

- `1`: passive
- `2`: PD hold the YAML `q_des`
- `3`: stand
- `4`: walk
- `5`: run
- `w/s`: forward velocity up/down
- `a/d`: lateral velocity left/right
- `j/l`: yaw velocity left/right
- `space`: zero velocity
- `q`: exit

FSM states and keyboard transitions are declared through `config.yaml` includes.
Passive and PD are simple YAML files under `config/states/`, while each RL FSM
has its own folder:

```text
config/states/stand/
  policy.yaml
  state.yaml
  policies/
config/states/walk/
  policy.yaml
  state.yaml
  policies/
config/states/run/
  policy.yaml
  state.yaml
  policies/
```

Use each `policy.yaml` to select the policy used by that FSM, and place the
corresponding ONNX files under that FSM's `policies/` directory. Relative
`model_path` values are resolved from the YAML file that declares them.

FSM declarations live under `include/fsm/`, and state implementations live under
`src/fsm/`:

- `passive_state.hpp` / `passive_state.cpp`: disables motor commands.
- `pd_state.hpp` / `pd_state.cpp`: holds the YAML `pd.q_des`.
- `rl_state.hpp` / `rl_state.cpp`: runs the joint-domain RL controller for
  `stand`, `walk`, and `run`.

A `type: pd` state reads `pd.q_des`, `pd.kp`, and `pd.kd` from YAML; `q_des`
must contain 29 motor targets, while `kp` and `kd` can be either a scalar for
all motors or 29 per-motor values. Policies currently support `type: zero` as
the safe placeholder; add another `JointPolicy` implementation in
`src/policy.cpp` to connect an ONNX or Torch policy without changing the FSM or
the per-state folder layout.
