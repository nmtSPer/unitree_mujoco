#include "types.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace g1 {
namespace {

namespace fs = std::filesystem;

bool isIncludeKey(const std::string& key) {
  return key == "include" || key == "includes";
}

bool hasUriScheme(const std::string& value) {
  return value.find("://") != std::string::npos;
}

void normalizePolicyModelPaths(YAML::Node yaml, const fs::path& base_dir) {
  const auto policies = yaml ? yaml["policies"] : YAML::Node();
  if (!policies || !policies.IsSequence()) {
    return;
  }

  for (auto policy : policies) {
    const auto model_path_node = policy["model_path"];
    if (!model_path_node || !model_path_node.IsScalar()) {
      continue;
    }

    const auto model_path_value = model_path_node.as<std::string>();
    if (model_path_value.empty() || hasUriScheme(model_path_value)) {
      continue;
    }

    fs::path model_path(model_path_value);
    if (model_path.is_relative()) {
      policy["model_path"] = (base_dir / model_path).lexically_normal().string();
    }
  }
}

YAML::Node mergeYamlNodes(const YAML::Node& base, const YAML::Node& overlay) {
  if (!overlay || overlay.IsNull()) {
    return YAML::Clone(base);
  }

  if (overlay.IsMap()) {
    YAML::Node merged = (base && base.IsMap()) ? YAML::Clone(base)
                                               : YAML::Node(YAML::NodeType::Map);
    for (const auto& item : overlay) {
      const auto key = item.first.as<std::string>();
      if (isIncludeKey(key)) {
        continue;
      }
      merged[key] = mergeYamlNodes(merged[key], item.second);
    }
    return merged;
  }

  if (overlay.IsSequence() && base && base.IsSequence()) {
    YAML::Node merged = YAML::Clone(base);
    for (const auto& item : overlay) {
      merged.push_back(YAML::Clone(item));
    }
    return merged;
  }

  return YAML::Clone(overlay);
}

void appendIncludePaths(const YAML::Node& include_node,
                        std::vector<std::string>& includes,
                        const std::string& context) {
  if (!include_node) {
    return;
  }
  if (include_node.IsScalar()) {
    includes.push_back(include_node.as<std::string>());
    return;
  }
  if (!include_node.IsSequence()) {
    throw std::runtime_error(context + " include must be a string or a list");
  }
  for (const auto& item : include_node) {
    if (!item.IsScalar()) {
      throw std::runtime_error(context + " include entries must be strings");
    }
    includes.push_back(item.as<std::string>());
  }
}

YAML::Node loadYamlWithIncludes(const fs::path& path, std::vector<fs::path>& stack) {
  const auto absolute_path = fs::absolute(path).lexically_normal();
  if (std::find(stack.begin(), stack.end(), absolute_path) != stack.end()) {
    throw std::runtime_error("recursive config include: " + absolute_path.string());
  }
  if (!fs::exists(absolute_path)) {
    throw std::runtime_error("config file not found: " + absolute_path.string());
  }

  stack.push_back(absolute_path);
  auto yaml = YAML::LoadFile(absolute_path.string());
  normalizePolicyModelPaths(yaml, absolute_path.parent_path());

  std::vector<std::string> includes;
  appendIncludePaths(yaml["include"], includes, absolute_path.string());
  appendIncludePaths(yaml["includes"], includes, absolute_path.string());

  YAML::Node merged(YAML::NodeType::Map);
  for (const auto& include : includes) {
    fs::path include_path(include);
    if (include_path.is_relative()) {
      include_path = absolute_path.parent_path() / include_path;
    }
    merged = mergeYamlNodes(merged, loadYamlWithIncludes(include_path, stack));
  }
  merged = mergeYamlNodes(merged, yaml);

  stack.pop_back();
  return merged;
}

void loadRequiredMotorArray(const YAML::Node& node, const char* key,
                            std::array<float, kNumMotors>& dst,
                            const std::string& context) {
  const auto src = node ? node[key] : YAML::Node();
  if (!src || !src.IsSequence()) {
    throw std::runtime_error(context + " requires '" + key + "' as a " +
                             std::to_string(kNumMotors) + "-element array");
  }
  if (static_cast<int>(src.size()) != kNumMotors) {
    throw std::runtime_error(context + " '" + key + "' must have exactly " +
                             std::to_string(kNumMotors) + " values");
  }
  for (int i = 0; i < kNumMotors; ++i) {
    dst[i] = src[i].as<float>();
  }
}

void loadRequiredMotorArrayOrScalar(const YAML::Node& node, const char* key,
                                    std::array<float, kNumMotors>& dst,
                                    const std::string& context) {
  const auto src = node ? node[key] : YAML::Node();
  if (!src) {
    throw std::runtime_error(context + " requires '" + key + "'");
  }
  if (src.IsScalar()) {
    dst.fill(src.as<float>());
    return;
  }
  if (!src.IsSequence()) {
    throw std::runtime_error(context + " '" + key +
                             "' must be a scalar or a " +
                             std::to_string(kNumMotors) + "-element array");
  }
  if (static_cast<int>(src.size()) != kNumMotors) {
    throw std::runtime_error(context + " '" + key + "' must have exactly " +
                             std::to_string(kNumMotors) + " values");
  }
  for (int i = 0; i < kNumMotors; ++i) {
    dst[i] = src[i].as<float>();
  }
}

void loadPdNode(const YAML::Node& item, StateConfig& state) {
  state.pd.q_des = state.controller.default_q;
  state.pd.kp = state.controller.kp;
  state.pd.kd = state.controller.kd;

  YAML::Node pd = item["pd"];
  if (!pd) {
    pd = item;
  }

  const std::string context = "PD state '" + state.name + "'";
  loadRequiredMotorArray(pd, "q_des", state.pd.q_des, context);
  loadRequiredMotorArrayOrScalar(pd, "kp", state.pd.kp, context);
  loadRequiredMotorArrayOrScalar(pd, "kd", state.pd.kd, context);
}

}  // namespace

RuntimeConfig loadConfig(const std::string& path) {
  RuntimeConfig config;
  setControllerDefaults(config.controller);

  std::vector<fs::path> include_stack;
  const auto yaml = loadYamlWithIncludes(path, include_stack);
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
  if (yaml["telemetry"]) {
    const auto telemetry = yaml["telemetry"];
    if (telemetry["enabled"]) config.telemetry.enabled = telemetry["enabled"].as<bool>();
    if (telemetry["host"]) config.telemetry.host = telemetry["host"].as<std::string>();
    if (telemetry["port"]) config.telemetry.port = telemetry["port"].as<int>();
    if (telemetry["decimation"]) {
      config.telemetry.decimation = std::max(1, telemetry["decimation"].as<int>());
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
      if (state.type == "pd") {
        loadPdNode(item, state);
      }
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
