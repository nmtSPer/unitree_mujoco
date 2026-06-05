#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "rl_controller.hpp"
#include "types.hpp"

namespace g1 {

class PolicyFactory {
 public:
  explicit PolicyFactory(const std::vector<PolicyConfig>& policies);
  std::unique_ptr<JointPolicy> create(const std::string& name) const;

 private:
  std::unordered_map<std::string, PolicyConfig> policies_;
};

}  // namespace g1
