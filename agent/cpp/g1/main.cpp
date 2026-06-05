#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "agent.hpp"
#include "types.hpp"

namespace {

std::atomic_bool g_running{true};

std::filesystem::path defaultConfigPath(const char* argv0) {
  const std::filesystem::path exe(argv0);
  const auto dir = exe.has_parent_path() ? exe.parent_path() : std::filesystem::path(".");
  return dir / ".." / "config.yaml";
}

std::string parseConfigPath(int argc, const char** argv) {
  auto path = defaultConfigPath(argv[0]).lexically_normal();
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      path = argv[++i];
    } else if (arg == "-h" || arg == "--help") {
      std::cout << "Usage: ./main [--config PATH]\n";
      std::exit(EXIT_SUCCESS);
    } else {
      std::cerr << "Unknown or incomplete argument: " << arg << "\n";
      std::exit(EXIT_FAILURE);
    }
  }
  return path.string();
}

void signalHandler(int) {
  g_running = false;
}

}  // namespace

int main(int argc, const char** argv) {
  ::signal(SIGINT, signalHandler);
  ::signal(SIGTERM, signalHandler);

  const auto config_path = parseConfigPath(argc, argv);
  const auto config = g1::loadConfig(config_path);

  std::cout << "[g1] config: " << config_path << "\n"
            << "[g1] DDS domain=" << config.domain_id
            << " interface=" << config.interface
            << " lowcmd=" << config.lowcmd_topic
            << " lowstate=" << config.lowstate_topic << "\n";

  g1::G1RlAgent agent(config);
  agent.init();

  if (config.wait_for_enter) {
    std::cout << "[g1] press enter to start FSM";
    std::cin.get();
  }
  agent.enter();

  while (g_running && !agent.stopRequested()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  agent.shutdown();
  return 0;
}
