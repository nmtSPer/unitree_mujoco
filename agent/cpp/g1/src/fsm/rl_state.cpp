#include "fsm/rl_state.hpp"

#include <utility>

namespace g1 {

RlState::RlState(StateConfig config, std::unique_ptr<JointPolicy> policy)
    : name_(config.name),
      controller_(std::move(config.controller), std::move(policy)) {}

const std::string& RlState::name() const {
  return name_;
}

void RlState::enter(ControllerContext& ctx, MotorCommand& out) {
  zeroCommand(out, true);
  controller_.enter(ctx);
}

void RlState::step(ControllerContext& ctx, MotorCommand& out) {
  controller_.step(ctx, out);
}

void RlState::exit(ControllerContext& ctx, MotorCommand& out) {
  controller_.exit(ctx, out);
}

}  // namespace g1
