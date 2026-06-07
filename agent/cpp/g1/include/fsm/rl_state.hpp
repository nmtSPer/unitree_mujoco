#pragma once

#include <memory>
#include <string>

#include "fsm/fsm.hpp"

namespace g1 {

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

}  // namespace g1
