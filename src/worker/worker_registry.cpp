#include "testgraph/worker/worker_registry.hpp"

namespace testgraph {
WorkerInfo WorkerRegistry::registerWorker(WorkerId id, double cpu, std::uint64_t memory) {
  std::lock_guard lock(mutex_);
  WorkerInfo info{std::move(id), WorkerState::Idle, std::chrono::steady_clock::now(), std::nullopt, cpu, memory};
  workers_[info.id] = info;
  return info;
}

bool WorkerRegistry::heartbeat(const WorkerId& id, std::optional<JobId> currentJob, double cpu, std::uint64_t memory) {
  std::lock_guard lock(mutex_);
  auto it = workers_.find(id);
  if (it == workers_.end() || it->second.state == WorkerState::Dead) return false;
  auto& w = it->second;
  w.lastHeartbeat = std::chrono::steady_clock::now();
  w.currentJob = currentJob;
  w.availableCpu = cpu;
  w.availableMemory = memory;
  w.state = currentJob ? WorkerState::Busy : WorkerState::Idle;
  return true;
}

std::vector<JobId> WorkerRegistry::markDeadAndCollectJobs(std::chrono::milliseconds timeout) {
  std::lock_guard lock(mutex_);
  std::vector<JobId> jobs;
  const auto now = std::chrono::steady_clock::now();
  for (auto& [_, w] : workers_) {
    if (w.state != WorkerState::Dead && now - w.lastHeartbeat > timeout) {
      if (w.currentJob) jobs.push_back(*w.currentJob);
      w.currentJob.reset();
      w.state = WorkerState::Dead;
    }
  }
  return jobs;
}

std::optional<WorkerInfo> WorkerRegistry::get(const WorkerId& id) const {
  std::lock_guard lock(mutex_);
  auto it = workers_.find(id);
  return it == workers_.end() ? std::nullopt : std::optional<WorkerInfo>(it->second);
}

std::vector<WorkerInfo> WorkerRegistry::list() const {
  std::lock_guard lock(mutex_);
  std::vector<WorkerInfo> out;
  for (const auto& [_, w] : workers_) out.push_back(w);
  return out;
}

bool CompletionGate::accept(JobId job, int attempt, std::string_view token) {
  std::lock_guard lock(mutex_);
  const auto key = std::to_string(job) + ":" + std::to_string(attempt) + ":" + std::string(token);
  return seen_.insert(key).second;
}
} // namespace testgraph
