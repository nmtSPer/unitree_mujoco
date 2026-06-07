#include "fsm/passive_state.hpp"

#include <utility>

namespace g1 {

PassiveState::PassiveState(std::string name) : name_(std::move(name)) {}

const std::string& PassiveState::name() const {
  return name_;
}

void PassiveState::enter(ControllerContext& ctx, MotorCommand& out) {
  ctx.rl_mode = false;
  zeroCommand(out, false);
}

void PassiveState::step(ControllerContext& ctx, MotorCommand& out) {
  ctx.rl_mode = false;
  zeroCommand(out, false);
}

void PassiveState::exit(ControllerContext&, MotorCommand& out) {
  zeroCommand(out, false);
}

}  // namespace g1
