#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace g1 {

constexpr int kNumMotors = 29;
constexpr float kPosStop = 2.146e9f;
constexpr float kVelStop = 16000.0f;

enum class AnkleMode : uint8_t {
  PR = 0,
  AB = 1,
};

enum JointIndex {
  LeftHipPitch = 0,
  LeftHipRoll = 1,
  LeftHipYaw = 2,
  LeftKnee = 3,
  LeftAnklePitch = 4,
  LeftAnkleRoll = 5,
  RightHipPitch = 6,
  RightHipRoll = 7,
  RightHipYaw = 8,
  RightKnee = 9,
  RightAnklePitch = 10,
  RightAnkleRoll = 11,
  WaistYaw = 12,
  WaistRoll = 13,
  WaistPitch = 14,
  LeftShoulderPitch = 15,
  RightWristYaw = 28,
};

struct VelocityCommand {
  float vx = 0.0f;
  float vy = 0.0f;
  float wz = 0.0f;
};

struct VelocityLimits {
  float vx = 0.8f;
  float vy = 0.4f;
  float wz = 1.0f;
};

struct KeyboardConfig {
  float vx_step = 0.05f;
  float vy_step = 0.05f;
  float wz_step = 0.05f;
};

struct InputCommand {
  VelocityCommand velocity;
  std::string requested_state;
  bool zero_velocity = false;
  bool exit = false;
};

struct MotorState {
  std::array<float, kNumMotors> q{};
  std::array<float, kNumMotors> dq{};
};

struct ImuState {
  std::array<float, 3> rpy{};
  std::array<float, 3> gyro{};
  std::array<float, 4> quat{};
};

struct RobotState {
  MotorState motor;
  ImuState imu;
  uint8_t mode_machine = 0;
};

struct MotorCommand {
  std::array<float, kNumMotors> q{};
  std::array<float, kNumMotors> dq{};
  std::array<float, kNumMotors> kp{};
  std::array<float, kNumMotors> kd{};
  std::array<float, kNumMotors> tau{};
  std::array<uint8_t, kNumMotors> mode{};
};

struct ControllerConfig {
  double control_dt = 0.002;
  AnkleMode ankle_mode = AnkleMode::PR;
  VelocityLimits limits;
  VelocityCommand default_velocity;
  std::array<float, kNumMotors> default_q{};
  std::array<float, kNumMotors> action_scale{};
  std::array<float, kNumMotors> kp{};
  std::array<float, kNumMotors> kd{};
  std::array<float, kNumMotors> q_min{};
  std::array<float, kNumMotors> q_max{};
  float action_clip = 1.0f;
};

struct PolicyConfig {
  std::string name;
  std::string type = "zero";
  std::string model_path;
};

struct PdConfig {
  std::array<float, kNumMotors> q_des{};
  std::array<float, kNumMotors> kp{};
  std::array<float, kNumMotors> kd{};
};

struct StateConfig {
  std::string name;
  std::string type = "rl";
  std::string policy;
  ControllerConfig controller;
  PdConfig pd;
};

struct FsmConfig {
  std::string initial_state = "passive";
  std::map<char, std::string> key_transitions;
};

struct TelemetryConfig {
  bool enabled = false;
  std::string host = "127.0.0.1";
  int port = 9870;
  int decimation = 1;
};

struct RuntimeConfig {
  int domain_id = 1;
  std::string interface = "lo";
  std::string lowcmd_topic = "rt/lowcmd";
  std::string lowstate_topic = "rt/lowstate";
  bool release_motion_service = true;
  bool wait_for_enter = true;
  TelemetryConfig telemetry;
  KeyboardConfig keyboard;
  ControllerConfig controller;
  FsmConfig fsm;
  std::vector<PolicyConfig> policies;
  std::vector<StateConfig> states;
};

template <typename T>
class DataBuffer {
 public:
  void set(const T& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_ = std::make_shared<T>(value);
  }

  std::shared_ptr<const T> get() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return data_;
  }

  void clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_.reset();
  }

 private:
  mutable std::shared_mutex mutex_;
  std::shared_ptr<T> data_;
};

inline float clamp(float value, float lo, float hi) {
  return std::max(lo, std::min(hi, value));
}

inline VelocityCommand clampVelocity(const VelocityCommand& command,
                                     const VelocityLimits& limits) {
  return {
      clamp(command.vx, -limits.vx, limits.vx),
      clamp(command.vy, -limits.vy, limits.vy),
      clamp(command.wz, -limits.wz, limits.wz),
  };
}

inline uint32_t crc32Core(uint32_t* ptr, uint32_t len) {
  uint32_t crc = 0xFFFFFFFF;
  constexpr uint32_t polynomial = 0x04c11db7;

  for (uint32_t i = 0; i < len; ++i) {
    uint32_t xbit = 1U << 31;
    const uint32_t data = ptr[i];
    for (uint32_t bit = 0; bit < 32; ++bit) {
      crc = (crc & 0x80000000) ? ((crc << 1) ^ polynomial) : (crc << 1);
      if (data & xbit) {
        crc ^= polynomial;
      }
      xbit >>= 1;
    }
  }

  return crc;
}

inline void zeroCommand(MotorCommand& command, bool enable = true) {
  command = {};
  for (int i = 0; i < kNumMotors; ++i) {
    command.q[i] = kPosStop;
    command.dq[i] = kVelStop;
    command.mode[i] = enable ? 1 : 0;
  }
}

inline void fillArray(const YAML::Node& node, const char* key,
                      std::array<float, kNumMotors>& dst) {
  if (!node || !node[key]) {
    return;
  }
  const auto src = node[key];
  const int n = std::min<int>(kNumMotors, static_cast<int>(src.size()));
  for (int i = 0; i < n; ++i) {
    dst[i] = src[i].as<float>();
  }
}

inline void fillScalarArray(float value, std::array<float, kNumMotors>& dst) {
  dst.fill(value);
}

inline void setControllerDefaults(ControllerConfig& controller) {
  fillScalarArray(1.0f, controller.action_scale);
  fillScalarArray(40.0f, controller.kp);
  fillScalarArray(1.0f, controller.kd);
  fillScalarArray(-3.14f, controller.q_min);
  fillScalarArray(3.14f, controller.q_max);
  controller.kp[LeftKnee] = 100.0f;
  controller.kp[RightKnee] = 100.0f;
  controller.kd[LeftKnee] = 2.0f;
  controller.kd[RightKnee] = 2.0f;
}

inline void loadControllerNode(const YAML::Node& ctrl, ControllerConfig& controller) {
  if (!ctrl) {
    return;
  }
  if (ctrl["dt"]) controller.control_dt = ctrl["dt"].as<double>();
  if (ctrl["action_clip"]) controller.action_clip = ctrl["action_clip"].as<float>();
  if (ctrl["ankle_mode"]) {
    controller.ankle_mode =
        ctrl["ankle_mode"].as<std::string>() == "AB" ? AnkleMode::AB : AnkleMode::PR;
  }
  if (ctrl["velocity_limits"]) {
    const auto limits = ctrl["velocity_limits"];
    if (limits["vx"]) controller.limits.vx = limits["vx"].as<float>();
    if (limits["vy"]) controller.limits.vy = limits["vy"].as<float>();
    if (limits["wz"]) controller.limits.wz = limits["wz"].as<float>();
  }
  if (ctrl["default_velocity"]) {
    const auto velocity = ctrl["default_velocity"];
    if (velocity["vx"]) controller.default_velocity.vx = velocity["vx"].as<float>();
    if (velocity["vy"]) controller.default_velocity.vy = velocity["vy"].as<float>();
    if (velocity["wz"]) controller.default_velocity.wz = velocity["wz"].as<float>();
  }
  fillArray(ctrl, "default_q", controller.default_q);
  fillArray(ctrl, "action_scale", controller.action_scale);
  fillArray(ctrl, "kp", controller.kp);
  fillArray(ctrl, "kd", controller.kd);
  fillArray(ctrl, "q_min", controller.q_min);
  fillArray(ctrl, "q_max", controller.q_max);
}

RuntimeConfig loadConfig(const std::string& path);


}  // namespace g1
