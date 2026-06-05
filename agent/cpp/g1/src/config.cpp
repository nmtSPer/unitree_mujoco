#include "types.hpp"

#include <stdexcept>

namespace g1 {

RuntimeConfig loadConfig(const std::string& path) {
  RuntimeConfig config;
  setControllerDefaults(config.controller);

  const auto yaml = YAML::LoadFile(path);
  if (yaml["dds"]) {
    const auto dds = yaml["dds"];
    if (dds["domain_id"]) config.domain_id = dds["domain_id"].as<int>();
    if (dds["interface"]) config.interface = dds["interface"].as<std::string>();
  }
  if (yaml["topics"]) {
    const auto topics = yaml["topics"];
    if (topics["lowcmd"]) config.lowcmd_topic = topics["lowcmd"].as<std::string>();
    if (topics["lowstate"]) config.lowstate_topic = topics["lowstate"].as<std::string>();
  }
  if (yaml["runtime"]) {
    const auto runtime = yaml["runtime"];
    if (runtime["release_motion_service"]) {
      config.release_motion_service = runtime["release_motion_service"].as<bool>();
    }
    if (runtime["wait_for_enter"]) {
      config.wait_for_enter = runtime["wait_for_enter"].as<bool>();
    }
  }
  if (yaml["keyboard"]) {
    const auto keyboard = yaml["keyboard"];
    if (keyboard["vx_step"]) config.keyboard.vx_step = keyboard["vx_step"].as<float>();
    if (keyboard["vy_step"]) config.keyboard.vy_step = keyboard["vy_step"].as<float>();
    if (keyboard["wz_step"]) config.keyboard.wz_step = keyboard["wz_step"].as<float>();
  }

  loadControllerNode(yaml["controller"], config.controller);

  if (yaml["policies"]) {
    for (const auto& item : yaml["policies"]) {
      PolicyConfig policy;
      if (item["name"]) policy.name = item["name"].as<std::string>();
      if (item["type"]) policy.type = item["type"].as<std::string>();
      if (item["model_path"]) policy.model_path = item["model_path"].as<std::string>();
      if (policy.name.empty()) {
        throw std::runtime_error("policy entry requires name");
      }
      config.policies.push_back(policy);
    }
  }

  if (yaml["states"]) {
    for (const auto& item : yaml["states"]) {
      StateConfig state;
      state.controller = config.controller;
      if (item["name"]) state.name = item["name"].as<std::string>();
      if (item["type"]) state.type = item["type"].as<std::string>();
      if (item["policy"]) state.policy = item["policy"].as<std::string>();
      loadControllerNode(item["controller"], state.controller);
      if (state.name.empty()) {
        throw std::runtime_error("state entry requires name");
      }
      config.states.push_back(state);
    }
  }

  if (yaml["fsm"]) {
    const auto fsm = yaml["fsm"];
    if (fsm["initial_state"]) {
      config.fsm.initial_state = fsm["initial_state"].as<std::string>();
    }
    if (fsm["transitions"]) {
      for (const auto& item : fsm["transitions"]) {
        const auto key = item["key"].as<std::string>();
        const auto target = item["to"].as<std::string>();
        if (!key.empty()) {
          config.fsm.key_transitions[key[0]] = target;
        }
      }
    }
  }

  if (config.policies.empty()) {
    config.policies.push_back({"zero", "zero", ""});
  }
  if (config.states.empty()) {
    StateConfig passive;
    passive.name = "passive";
    passive.type = "passive";
    passive.controller = config.controller;
    config.states.push_back(passive);

    StateConfig walk;
    walk.name = "walk";
    walk.type = "rl";
    walk.policy = "zero";
    walk.controller = config.controller;
    config.states.push_back(walk);
  }

  return config;
}

}  // namespace g1
