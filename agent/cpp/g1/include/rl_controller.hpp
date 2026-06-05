#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "types.hpp"

namespace g1 {

struct ControllerContext {
  RobotState state;
  VelocityCommand command;
  bool rl_mode = false;
};

class G1Controller {
 public:
  virtual ~G1Controller() = default;
  virtual void enter(ControllerContext& ctx) = 0;
  virtual void step(ControllerContext& ctx, MotorCommand& out) = 0;
  virtual void exit(ControllerContext& ctx, MotorCommand& out) = 0;
};

class JointPolicy {
 public:
  virtual ~JointPolicy() = default;
  virtual void run(const std::vector<float>& observation,
                   std::array<float, kNumMotors>& action) = 0;
};

class ZeroJointPolicy final : public JointPolicy {
 public:
  void run(const std::vector<float>&, std::array<float, kNumMotors>& action) override {
    action.fill(0.0f);
  }
};

class JointDomainRlController final : public G1Controller {
 public:
  JointDomainRlController(ControllerConfig config, std::unique_ptr<JointPolicy> policy)
      : config_(std::move(config)), policy_(std::move(policy)) {
    last_action_.fill(0.0f);
  }

  void enter(ControllerContext& ctx) override {
    ctx.rl_mode = true;
    last_action_.fill(0.0f);
  }

  void step(ControllerContext& ctx, MotorCommand& out) override {
    const auto observation = buildObservation(ctx);
    policy_->run(observation, last_action_);
    applyJointAction(ctx, out);
  }

  void exit(ControllerContext& ctx, MotorCommand& out) override {
    ctx.rl_mode = false;
    zeroCommand(out, false);
    last_action_.fill(0.0f);
  }

 private:
  std::vector<float> buildObservation(const ControllerContext& ctx) const {
    std::vector<float> obs;
    obs.reserve(3 + 3 + 3 + kNumMotors * 3);

    obs.push_back(ctx.command.vx);
    obs.push_back(ctx.command.vy);
    obs.push_back(ctx.command.wz);
    obs.insert(obs.end(), ctx.state.imu.rpy.begin(), ctx.state.imu.rpy.end());
    obs.insert(obs.end(), ctx.state.imu.gyro.begin(), ctx.state.imu.gyro.end());

    for (int i = 0; i < kNumMotors; ++i) {
      obs.push_back(ctx.state.motor.q[i] - config_.default_q[i]);
    }
    obs.insert(obs.end(), ctx.state.motor.dq.begin(), ctx.state.motor.dq.end());
    obs.insert(obs.end(), last_action_.begin(), last_action_.end());
    return obs;
  }

  void applyJointAction(const ControllerContext&, MotorCommand& out) const {
    for (int i = 0; i < kNumMotors; ++i) {
      const float action = clamp(last_action_[i], -config_.action_clip, config_.action_clip);
      const float q_des = config_.default_q[i] + action * config_.action_scale[i];
      out.q[i] = clamp(q_des, config_.q_min[i], config_.q_max[i]);
      out.dq[i] = 0.0f;
      out.kp[i] = config_.kp[i];
      out.kd[i] = config_.kd[i];
      out.tau[i] = 0.0f;
      out.mode[i] = 1;
    }
  }

  ControllerConfig config_;
  std::unique_ptr<JointPolicy> policy_;
  std::array<float, kNumMotors> last_action_;
};

}  // namespace g1
