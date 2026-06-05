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

class PassiveState final : public FsmState {
 public:
  explicit PassiveState(std::string name);
  const std::string& name() const override;
  void enter(ControllerContext& ctx, MotorCommand& out) override;
  void step(ControllerContext& ctx, MotorCommand& out) override;
  void exit(ControllerContext& ctx, MotorCommand& out) override;

 private:
  std::string name_;
};

class RlState final : public FsmState {
 public:
  RlState(StateConfig config, std::unique_ptr<JointPolicy> policy);
  const std::string& name() const override;
  void enter(ControllerContext& ctx, MotorCommand& out) override;
  void step(ControllerContext& ctx, MotorCommand& out) override;
  void exit(ControllerContext& ctx, MotorCommand& out) override;

 private:
  std::string name_;
  JointDomainRlController controller_;
};

class Fsm {
 public:
  Fsm(const RuntimeConfig& config, const PolicyFactory& policy_factory);
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
