#include "testgraph/scheduler/scheduler.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <unordered_set>

namespace testgraph {
void JobGraph::add(JobNode job) { jobs_[job.id] = std::move(job); }

void JobGraph::addDependency(JobId job, JobId dependsOn) {
  if (!jobs_.contains(job) || !jobs_.contains(dependsOn)) throw std::invalid_argument("unknown job id");
  auto& deps = jobs_[job].dependencies;
  if (std::find(deps.begin(), deps.end(), dependsOn) == deps.end()) deps.push_back(dependsOn);
  auto& rev = jobs_[dependsOn].dependents;
  if (std::find(rev.begin(), rev.end(), job) == rev.end()) rev.push_back(job);
}

std::vector<JobId> JobGraph::topologicalOrder() const {
  std::unordered_map<JobId, std::size_t> indegree;
  std::queue<JobId> ready;
  for (const auto& [id, j] : jobs_) {
    indegree[id] = j.dependencies.size();
    if (j.dependencies.empty()) ready.push(id);
  }
  std::vector<JobId> out;
  while (!ready.empty()) {
    auto id = ready.front(); ready.pop(); out.push_back(id);
    for (auto d : jobs_.at(id).dependents) if (--indegree[d] == 0) ready.push(d);
  }
  if (out.size() != jobs_.size()) throw std::runtime_error("job graph contains a cycle");
  return out;
}

std::unordered_map<JobId, std::chrono::milliseconds> JobGraph::criticalPathWeights() const {
  auto order = topologicalOrder();
  std::unordered_map<JobId, std::chrono::milliseconds> weight;
  for (auto it = order.rbegin(); it != order.rend(); ++it) {
    const auto& job = jobs_.at(*it);
    std::chrono::milliseconds childMax{0};
    for (auto child : job.dependents) childMax = std::max(childMax, weight[child]);
    weight[job.id] = job.estimatedDuration + childMax;
  }
  return weight;
}

std::vector<WorkerAssignment> Scheduler::planLpt(const std::vector<JobNode>& independentJobs, std::size_t workerCount) {
  if (workerCount == 0) throw std::invalid_argument("workerCount must be > 0");
  auto jobs = independentJobs;
  std::sort(jobs.begin(), jobs.end(), [](const auto& a, const auto& b) {
    if (a.estimatedDuration != b.estimatedDuration) return a.estimatedDuration > b.estimatedDuration;
    return a.priority > b.priority;
  });
  std::vector<std::chrono::milliseconds> loads(workerCount, std::chrono::milliseconds{0});
  std::vector<WorkerAssignment> result;
  for (const auto& j : jobs) {
    const auto it = std::min_element(loads.begin(), loads.end());
    const auto idx = static_cast<std::size_t>(std::distance(loads.begin(), it));
    const auto start = loads[idx];
    loads[idx] += j.estimatedDuration;
    result.push_back({idx, j.id, start, loads[idx]});
  }
  return result;
}

std::vector<WorkerAssignment> Scheduler::planDag(const JobGraph& graph, std::size_t workerCount) {
  if (workerCount == 0) throw std::invalid_argument("workerCount must be > 0");
  const auto critical = graph.criticalPathWeights();
  std::unordered_map<JobId, std::chrono::milliseconds> finish;
  std::vector<std::chrono::milliseconds> workerFree(workerCount, std::chrono::milliseconds{0});
  std::vector<WorkerAssignment> out;
  for (auto id : graph.topologicalOrder()) {
    const auto& j = graph.jobs().at(id);
    std::chrono::milliseconds depsReady{0};
    for (auto dep : j.dependencies) depsReady = std::max(depsReady, finish[dep]);
    std::size_t bestWorker = 0;
    auto bestStart = std::max(workerFree[0], depsReady);
    for (std::size_t w = 1; w < workerCount; ++w) {
      auto candidate = std::max(workerFree[w], depsReady);
      if (candidate < bestStart) { bestStart = candidate; bestWorker = w; }
    }
    const auto done = bestStart + j.estimatedDuration;
    workerFree[bestWorker] = done;
    finish[id] = done;
    out.push_back({bestWorker, id, bestStart, done});
  }
  std::sort(out.begin(), out.end(), [&](const auto& a, const auto& b) {
    if (a.predictedStart != b.predictedStart) return a.predictedStart < b.predictedStart;
    return critical.at(a.jobId) > critical.at(b.jobId);
  });
  return out;
}
} // namespace testgraph
