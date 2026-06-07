#pragma once

#include <atomic>

#include <unitree/common/thread/thread.hpp>
#include <unitree/idl/hg/LowCmd_.hpp>
#include <unitree/idl/hg/LowState_.hpp>
#include <unitree/robot/channel/channel_publisher.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>

#include "fsm/fsm.hpp"
#include "keyboard.hpp"
#include "policy.hpp"
#include "types.hpp"

namespace g1 {

class G1RlAgent {
 public:
  explicit G1RlAgent(RuntimeConfig config);
  void init();
  void enter();
  void shutdown();
  bool stopRequested() const;

 private:
  void run();
  void lowStateHandler(const void* message);
  void writeLowCommand();
  void releaseMotionService();

  RuntimeConfig config_;
  PolicyFactory policy_factory_;
  FSM fsm_;
  KeyboardInput keyboard_;
  ControllerContext context_;
  DataBuffer<RobotState> state_buffer_;
  DataBuffer<MotorCommand> command_buffer_;
  std::atomic_bool stop_requested_{false};
  std::atomic<uint8_t> latest_mode_machine_{0};
  unitree::robot::ChannelPublisherPtr<unitree_hg::msg::dds_::LowCmd_> lowcmd_publisher_;
  unitree::robot::ChannelSubscriberPtr<unitree_hg::msg::dds_::LowState_> lowstate_subscriber_;
  unitree::common::ThreadPtr writer_thread_;
  unitree::common::ThreadPtr control_thread_;
};

}  // namespace g1
