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
struct WorkerInfo {
  WorkerId id;
  WorkerState state{WorkerState::Registering};
  std::chrono::steady_clock::time_point lastHeartbeat{std::chrono::steady_clock::now()};
  std::optional<JobId> currentJob;
  double availableCpu{0};
  std::uint64_t availableMemory{0};
};

class WorkerRegistry {
 public:
  WorkerInfo registerWorker(WorkerId id, double cpu, std::uint64_t memory);
  bool heartbeat(const WorkerId& id, std::optional<JobId> currentJob, double cpu, std::uint64_t memory);
  std::vector<JobId> markDeadAndCollectJobs(std::chrono::milliseconds timeout);
  std::optional<WorkerInfo> get(const WorkerId& id) const;
  std::vector<WorkerInfo> list() const;
 private:
  mutable std::mutex mutex_;
  std::unordered_map<WorkerId, WorkerInfo> workers_;
};

class CompletionGate {
 public:
  bool accept(JobId job, int attempt, std::string_view executionToken);
 private:
  std::mutex mutex_;
  std::unordered_set<std::string> seen_;
};
} // namespace testgraph
