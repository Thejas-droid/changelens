#pragma once
#include "testgraph/cache/cache.hpp"
#include "testgraph/config/config.hpp"
#include "testgraph/domain.hpp"
#include "testgraph/executor/process_executor.hpp"
#include "testgraph/graph/dependency_graph.hpp"
#include "testgraph/impact/impact_analyzer.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
namespace testgraph {
struct LocalRunEnvironment {
  std::string testRunnerVersion{"unknown"};
  std::string runtimeVersion{"unknown"};
  std::string buildConfiguration{"default"};
  std::string environmentFingerprint{"local"};
};
struct TestRunRecord {
  TestId testId{};
  TestStatus status{TestStatus::Error};
  int attempts{0};
  bool cacheHit{false};
  bool probableFlaky{false};
  std::chrono::milliseconds duration{0};
  std::string output;
  std::string failureSignature;
};
struct LocalRunResult {
  ImpactResult impact;
  std::vector<TestRunRecord> tests;
  std::chrono::milliseconds wallTime{0};
  std::size_t cacheHits{0};
  std::size_t flakyCount{0};
  bool passed{true};
};
class LocalRunCoordinator {
 public:
  LocalRunCoordinator(Config config, TestCache& cache) : config_(std::move(config)), cache_(cache) {}
  LocalRunResult run(const std::filesystem::path& repositoryRoot,
                     const DependencyGraph& graph,
                     const std::vector<TestCase>& tests,
                     const std::vector<std::filesystem::path>& changedPaths,
                     const std::unordered_map<TestId,std::chrono::milliseconds>& estimates = {},
                     LocalRunEnvironment environment = {},
                     const CancellationToken* cancellation = nullptr) const;
 private:
  Config config_;
  TestCache& cache_;
};
}
