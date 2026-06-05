#include "agent.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include <unitree/robot/b2/motion_switcher/motion_switcher_client.hpp>
#include <unitree/robot/channel/channel_factory.hpp>

namespace g1 {

G1RlAgent::G1RlAgent(RuntimeConfig config)
    : config_(std::move(config)),
      policy_factory_(config_.policies),
      fsm_(config_, policy_factory_),
      keyboard_(config_.keyboard, config_.controller.limits, config_.fsm.key_transitions) {}

void G1RlAgent::init() {
  unitree::robot::ChannelFactory::Instance()->Init(config_.domain_id, config_.interface);
  if (config_.release_motion_service) {
    releaseMotionService();
  }

  MotorCommand zero;
  zeroCommand(zero, false);
  command_buffer_.set(zero);

  lowcmd_publisher_.reset(
      new unitree::robot::ChannelPublisher<unitree_hg::msg::dds_::LowCmd_>(
          config_.lowcmd_topic));
  lowcmd_publisher_->InitChannel();

  lowstate_subscriber_.reset(
      new unitree::robot::ChannelSubscriber<unitree_hg::msg::dds_::LowState_>(
          config_.lowstate_topic));
  lowstate_subscriber_->InitChannel(
      std::bind(&G1RlAgent::lowStateHandler, this, std::placeholders::_1), 1);

  const int period_us = static_cast<int>(config_.controller.control_dt * 1000000.0);
  writer_thread_ = unitree::common::CreateRecurrentThreadEx(
      "lowcmd_writer", UT_CPU_ID_NONE, period_us, &G1RlAgent::writeLowCommand, this);
  control_thread_ = unitree::common::CreateRecurrentThreadEx(
      "fsm_control", UT_CPU_ID_NONE, period_us, &G1RlAgent::run, this);
}

void G1RlAgent::enter() {
  context_.command = clampVelocity(config_.controller.default_velocity, config_.controller.limits);

  MotorCommand command;
  fsm_.enter(context_, command);
  command_buffer_.set(command);
  keyboard_.start();
}

void G1RlAgent::shutdown() {
  keyboard_.stop();

  MotorCommand command;
  fsm_.exit(context_, command);
  command_buffer_.set(command);
  writeLowCommand();
}

bool G1RlAgent::stopRequested() const {
  return stop_requested_;
}

void G1RlAgent::run() {
  const auto state = state_buffer_.get();
  if (!state) {
    return;
  }

  const auto input = keyboard_.snapshot();
  if (input.exit) {
    stop_requested_ = true;
    return;
  }

  context_.state = *state;
  context_.command = clampVelocity(input.velocity, config_.controller.limits);

  MotorCommand command;
  fsm_.step(input, context_, command);
  command_buffer_.set(command);
}

void G1RlAgent::lowStateHandler(const void* message) {
  auto low_state = *(const unitree_hg::msg::dds_::LowState_*)message;
  const uint32_t crc = crc32Core(
      (uint32_t*)&low_state, (sizeof(unitree_hg::msg::dds_::LowState_) >> 2) - 1);
  if (low_state.crc() != crc) {
    std::cerr << "[g1] lowstate CRC error\n";
    return;
  }

  RobotState state;
  state.mode_machine = low_state.mode_machine();
  state.imu.rpy = low_state.imu_state().rpy();
  state.imu.gyro = low_state.imu_state().gyroscope();
  state.imu.quat = low_state.imu_state().quaternion();
  for (int i = 0; i < kNumMotors; ++i) {
    state.motor.q[i] = low_state.motor_state()[i].q();
    state.motor.dq[i] = low_state.motor_state()[i].dq();
  }

  latest_mode_machine_ = state.mode_machine;
  state_buffer_.set(state);
}

void G1RlAgent::writeLowCommand() {
  const auto command = command_buffer_.get();
  if (!command) {
    return;
  }

  unitree_hg::msg::dds_::LowCmd_ low_cmd;
  low_cmd.mode_pr() = static_cast<uint8_t>(config_.controller.ankle_mode);
  low_cmd.mode_machine() = latest_mode_machine_.load();
  for (int i = 0; i < kNumMotors; ++i) {
    auto& motor = low_cmd.motor_cmd().at(i);
    motor.mode() = command->mode[i];
    motor.q() = command->q[i];
    motor.dq() = command->dq[i];
    motor.kp() = command->kp[i];
    motor.kd() = command->kd[i];
    motor.tau() = command->tau[i];
  }

  low_cmd.crc() = crc32Core(
      (uint32_t*)&low_cmd, (sizeof(unitree_hg::msg::dds_::LowCmd_) >> 2) - 1);
  lowcmd_publisher_->Write(low_cmd);
}

void G1RlAgent::releaseMotionService() {
  auto msc = std::make_shared<unitree::robot::b2::MotionSwitcherClient>();
  msc->SetTimeout(5.0f);
  msc->Init();

  std::string form;
  std::string name;
  while (!stop_requested_) {
    const int32_t ret = msc->CheckMode(form, name);
    if (ret != 0 || name.empty()) {
      return;
    }
    std::cout << "[g1] releasing active motion service: " << name << "\n";
    msc->ReleaseMode();
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

}  // namespace g1
