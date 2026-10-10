#pragma once

#include "testgraph/config/config.hpp"
#include "testgraph/domain.hpp"
#include "testgraph/graph/dependency_graph.hpp"
#include <filesystem>
#include <vector>

namespace testgraph {
class TestDiscovery {
 public:
  explicit TestDiscovery(const Config& config) : config_(config) {}
  std::vector<TestCase> discover(const std::filesystem::path& repositoryRoot, const DependencyGraph& graph) const;
  bool isTestPath(const std::filesystem::path& relativePath) const;
 private:
  Config config_;
};
} // namespace testgraph
