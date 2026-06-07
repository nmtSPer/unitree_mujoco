#include "fsm/pd_state.hpp"

#include <utility>

namespace g1 {

PdState::PdState(StateConfig config)
    : name_(std::move(config.name)), config_(config.pd) {}

const std::string& PdState::name() const {
  return name_;
}

void PdState::enter(ControllerContext& ctx, MotorCommand& out) {
  ctx.rl_mode = false;
  apply(out);
}

void PdState::step(ControllerContext& ctx, MotorCommand& out) {
  ctx.rl_mode = false;
  apply(out);
}

void PdState::exit(ControllerContext&, MotorCommand& out) {
  zeroCommand(out, false);
}

void PdState::apply(MotorCommand& out) const {
  for (int i = 0; i < kNumMotors; ++i) {
    out.q[i] = config_.q_des[i];
    out.dq[i] = 0.0f;
    out.kp[i] = config_.kp[i];
    out.kd[i] = config_.kd[i];
    out.tau[i] = 0.0f;
    out.mode[i] = 1;
  }
}

}  // namespace g1
