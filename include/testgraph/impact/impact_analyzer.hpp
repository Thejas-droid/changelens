#pragma once

#include "testgraph/config/config.hpp"
#include "testgraph/domain.hpp"
#include "testgraph/graph/dependency_graph.hpp"
#include <filesystem>
#include <vector>

namespace testgraph {
class ImpactAnalyzer {
 public:
  explicit ImpactAnalyzer(Config config = Config::defaults()) : config_(std::move(config)) {}
  ImpactResult analyze(const DependencyGraph& graph, const std::vector<TestCase>& tests,
                       const std::vector<std::filesystem::path>& changedPaths,
                       Confidence parserConfidence = Confidence::High) const;
 private:
  Config config_;
};
} // namespace testgraph
