#pragma once

#include "testgraph/domain.hpp"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace testgraph {
struct RunRecord {
  std::string id;
  std::string repositoryId;
  int pullRequestNumber{0};
  std::string baseSha;
  std::string headSha;
  RunStatus status{RunStatus::Queued};
  Confidence confidence{Confidence::High};
  bool fullSuiteFallback{false};
  std::chrono::system_clock::time_point startedAt{std::chrono::system_clock::now()};
  std::optional<std::chrono::system_clock::time_point> completedAt;
};

struct TestExecutionRecord {
  std::string runId;
  TestId testId{};
  JobId jobId{};
  WorkerId workerId;
  TestStatus status{TestStatus::Error};
  std::chrono::milliseconds duration{0};
  int attempt{1};
  bool cacheHit{false};
  std::string failureSignature;
};

class InMemoryPersistence {
 public:
  bool recordWebhookDelivery(std::string deliveryId);
  void saveRun(RunRecord run);
  std::optional<RunRecord> run(std::string_view id) const;
  std::vector<RunRecord> runs() const;
  void saveExecution(TestExecutionRecord record);
  std::vector<TestExecutionRecord> executions(std::string_view runId) const;
 private:
  mutable std::mutex mutex_;
  std::unordered_set<std::string> deliveries_;
  std::unordered_map<std::string, RunRecord> runs_;
  std::vector<TestExecutionRecord> executions_;
};
} // namespace testgraph
