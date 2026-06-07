#pragma once

#include <string>

#include "fsm/fsm.hpp"

namespace g1 {

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

}  // namespace g1
