#pragma once

#include <string>

#include "fsm/fsm.hpp"

namespace g1 {

class PdState final : public FsmState {
 public:
  explicit PdState(StateConfig config);
  const std::string& name() const override;
  void enter(ControllerContext& ctx, MotorCommand& out) override;
  void step(ControllerContext& ctx, MotorCommand& out) override;
  void exit(ControllerContext& ctx, MotorCommand& out) override;

 private:
  void apply(MotorCommand& out) const;

  std::string name_;
  PdConfig config_;
};

}  // namespace g1
