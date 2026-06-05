#include "FSM.hpp"

#include <iostream>
#include <stdexcept>
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
  zeroCommand(out, true);
}

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

FSM::FSM(const RuntimeConfig& config, const PolicyFactory& policy_factory)
    : initial_state_(config.fsm.initial_state) {
  for (const auto& state : config.states) {
    if (state.type == "passive") {
      states_[state.name] = std::make_unique<PassiveState>(state.name);
    } else if (state.type == "rl") {
      if (state.policy.empty()) {
        throw std::runtime_error("RL state requires policy: " + state.name);
      }
      states_[state.name] = std::make_unique<RlState>(state, policy_factory.create(state.policy));
    } else {
      throw std::runtime_error("unsupported state type for '" + state.name + "': " + state.type);
    }
  }
  requireState(initial_state_);
}

void FSM::enter(ControllerContext& ctx, MotorCommand& out) {
  current_ = &requireState(initial_state_);
  std::cout << "[g1] FSM enter: " << current_->name() << "\n";
  current_->enter(ctx, out);
}

void FSM::step(const InputCommand& input, ControllerContext& ctx, MotorCommand& out) {
  if (!current_) {
    enter(ctx, out);
  }

  if (!input.requested_state.empty() && input.requested_state != current_->name()) {
    transitionTo(input.requested_state, ctx, out);
  }

  current_->step(ctx, out);
}

void FSM::exit(ControllerContext& ctx, MotorCommand& out) {
  if (current_) {
    current_->exit(ctx, out);
  } else {
    zeroCommand(out, false);
  }
  current_ = nullptr;
}

const std::string& FSM::currentStateName() const {
  static const std::string none = "none";
  return current_ ? current_->name() : none;
}

FsmState& FSM::requireState(const std::string& name) {
  const auto it = states_.find(name);
  if (it == states_.end()) {
    throw std::runtime_error("unknown FSM state: " + name);
  }
  return *it->second;
}

void FSM::transitionTo(const std::string& target, ControllerContext& ctx, MotorCommand& out) {
  auto& next = requireState(target);
  std::cout << "[g1] FSM transition: " << current_->name() << " -> " << next.name() << "\n";
  current_->exit(ctx, out);
  current_ = &next;
  current_->enter(ctx, out);
}

}  // namespace g1
