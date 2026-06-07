#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "policy.hpp"
#include "rl_controller.hpp"
#include "types.hpp"

namespace g1 {

class FsmState {
 public:
  virtual ~FsmState() = default;
  virtual const std::string& name() const = 0;
  virtual void enter(ControllerContext& ctx, MotorCommand& out) = 0;
  virtual void step(ControllerContext& ctx, MotorCommand& out) = 0;
  virtual void exit(ControllerContext& ctx, MotorCommand& out) = 0;
};

class FSM {
 public:
  FSM(const RuntimeConfig& config, const PolicyFactory& policy_factory);
  void enter(ControllerContext& ctx, MotorCommand& out);
  void step(const InputCommand& input, ControllerContext& ctx, MotorCommand& out);
  void exit(ControllerContext& ctx, MotorCommand& out);
  const std::string& currentStateName() const;

 private:
  FsmState& requireState(const std::string& name);
  void transitionTo(const std::string& target, ControllerContext& ctx, MotorCommand& out);

  std::string initial_state_;
  FsmState* current_ = nullptr;
  std::unordered_map<std::string, std::unique_ptr<FsmState>> states_;
};

}  // namespace g1
