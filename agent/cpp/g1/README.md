# G1 C++ Controllers

Minimal G1 low-level controller scaffold for joint-domain RL.

Build:

```bash
./agent/cpp/g1/build.sh
```

Run:

```bash
./agent/cpp/g1/build/main --config ./agent/cpp/g1/config.yaml
```

Keyboard:

- `1`: passive
- `2`: stand
- `3`: walk
- `w/s`: forward velocity up/down
- `a/d`: lateral velocity left/right
- `j/l`: yaw velocity left/right
- `space`: zero velocity
- `q`: exit

FSM states, keyboard transitions, and policy selection are declared in
`config.yaml`. Policies currently support `type: zero` as the safe placeholder;
add another `JointPolicy` implementation in `src/policy.cpp` to connect an
ONNX or Torch policy without changing the FSM.
