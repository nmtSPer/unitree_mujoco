#include "keyboard.hpp"

#include <algorithm>
#include <iostream>
#include <sys/select.h>
#include <unistd.h>
#include <utility>

namespace g1 {

KeyboardInput::KeyboardInput(KeyboardConfig config, VelocityLimits limits,
                             std::map<char, std::string> transitions)
    : config_(config), limits_(limits), transitions_(std::move(transitions)) {}

KeyboardInput::~KeyboardInput() {
  stop();
}

void KeyboardInput::start() {
  if (running_) {
    return;
  }

  tcgetattr(fileno(stdin), &old_settings_);
  auto new_settings = old_settings_;
  new_settings.c_lflag &= static_cast<unsigned>(~(ICANON | ECHO));
  tcsetattr(fileno(stdin), TCSANOW, &new_settings);
  terminal_configured_ = true;

  running_ = true;
  thread_ = std::thread(&KeyboardInput::run, this);
}

void KeyboardInput::stop() {
  running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
  restoreTerminal();
}

InputCommand KeyboardInput::snapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return command_;
}

void KeyboardInput::run() {
  while (running_) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fileno(stdin), &set);

    timeval timeout{};
    timeout.tv_usec = 100000;
    const int ready = select(fileno(stdin) + 1, &set, nullptr, nullptr, &timeout);
    if (ready <= 0) {
      continue;
    }

    char key = '\0';
    if (read(fileno(stdin), &key, 1) == 1) {
      handleKey(key);
    }
  }
}

void KeyboardInput::handleKey(char key) {
  std::lock_guard<std::mutex> lock(mutex_);
  command_.requested_state.clear();
  command_.zero_velocity = false;

  const auto transition = transitions_.find(key);
  if (transition != transitions_.end()) {
    command_.requested_state = transition->second;
    return;
  }

  switch (key) {
    case 'w':
    case 'W':
      command_.velocity.vx += config_.vx_step;
      break;
    case 's':
    case 'S':
      command_.velocity.vx -= config_.vx_step;
      break;
    case 'a':
    case 'A':
      command_.velocity.vy += config_.vy_step;
      break;
    case 'd':
    case 'D':
      command_.velocity.vy -= config_.vy_step;
      break;
    case 'j':
    case 'J':
      command_.velocity.wz += config_.wz_step;
      break;
    case 'l':
    case 'L':
      command_.velocity.wz -= config_.wz_step;
      break;
    case ' ':
      command_.velocity = {};
      command_.zero_velocity = true;
      break;
    case 'q':
    case 'Q':
      command_.exit = true;
      running_ = false;
      break;
    default:
      break;
  }

  command_.velocity = clampVelocity(command_.velocity, limits_);
}

void KeyboardInput::restoreTerminal() {
  if (terminal_configured_) {
    tcsetattr(fileno(stdin), TCSANOW, &old_settings_);
    terminal_configured_ = false;
  }
}

}  // namespace g1
