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

Run with the simulator tmux layout:

```bash
./start.sh robot=g1 scene=scene.xml
```

Keyboard:

- `1`: passive
- `2`: PD hold the YAML `q_des`
- `3`: stand
- `4`: walk
- `w/s`: forward velocity up/down
- `a/d`: lateral velocity left/right
- `j/l`: yaw velocity left/right
- `space`: zero velocity
- `q`: exit

FSM states, keyboard transitions, and policy selection are declared in
`config.yaml`. FSM declarations live under `include/fsm/`, and state
implementations live under `src/fsm/`:

- `passive_state.hpp` / `passive_state.cpp`: disables motor commands.
- `pd_state.hpp` / `pd_state.cpp`: holds the YAML `pd.q_des`.
- `rl_state.hpp` / `rl_state.cpp`: runs the joint-domain RL controller.

A `type: pd` state reads `pd.q_des`, `pd.kp`, and `pd.kd` from YAML; `q_des`
must contain 29 motor targets, while `kp` and `kd` can be either a scalar for
all motors or 29 per-motor values. Policies currently support `type: zero` as
the safe placeholder; add another `JointPolicy` implementation in
`src/policy.cpp` to connect an ONNX or Torch policy without changing the FSM.
