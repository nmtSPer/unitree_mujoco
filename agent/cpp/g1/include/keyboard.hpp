#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <termios.h>
#include <thread>

#include "types.hpp"

namespace g1 {

class KeyboardInput {
 public:
  KeyboardInput(KeyboardConfig config, VelocityLimits limits,
                std::map<char, std::string> transitions);
  ~KeyboardInput();

  KeyboardInput(const KeyboardInput&) = delete;
  KeyboardInput& operator=(const KeyboardInput&) = delete;

  void start();
  void stop();
  InputCommand snapshot() const;

 private:
  void run();
  void handleKey(char key);
  void restoreTerminal();

  KeyboardConfig config_;
  VelocityLimits limits_;
  std::map<char, std::string> transitions_;
  mutable std::mutex mutex_;
  InputCommand command_;
  std::atomic_bool running_{false};
  std::thread thread_;
  termios old_settings_{};
  bool terminal_configured_ = false;
};

}  // namespace g1
