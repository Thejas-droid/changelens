#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace testgraph {
struct CheckSummary {
  std::string status{"Passed"};
  std::size_t changedFiles{};
  std::size_t affectedFiles{};
  std::size_t repositoryTests{};
  std::size_t affectedTests{};
  std::size_t cacheHits{};
  std::size_t executedTests{};
  std::size_t passed{};
  std::size_t probableFlaky{};
  std::size_t failed{};
  std::chrono::milliseconds fullSuiteEstimate{0};
  std::chrono::milliseconds wallTime{0};
};
class GitHubService {
 public:
  static std::string formatCheckSummary(const CheckSummary& summary);
};
} // namespace testgraph
