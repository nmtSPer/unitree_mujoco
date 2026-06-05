#include "policy.hpp"

#include <stdexcept>

namespace g1 {

PolicyFactory::PolicyFactory(const std::vector<PolicyConfig>& policies) {
  for (const auto& policy : policies) {
    policies_[policy.name] = policy;
  }
}

std::unique_ptr<JointPolicy> PolicyFactory::create(const std::string& name) const {
  const auto it = policies_.find(name);
  if (it == policies_.end()) {
    throw std::runtime_error("unknown policy: " + name);
  }

  const auto& policy = it->second;
  if (policy.type == "zero") {
    return std::make_unique<ZeroJointPolicy>();
  }

  throw std::runtime_error("unsupported policy type for '" + name + "': " + policy.type);
}

}  // namespace g1
