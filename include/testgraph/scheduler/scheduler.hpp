#pragma once

#include "testgraph/domain.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

namespace testgraph {
struct WorkerAssignment {
  std::size_t workerIndex{};
  JobId jobId{};
  std::chrono::milliseconds predictedStart{0};
  std::chrono::milliseconds predictedFinish{0};
};

class JobGraph {
 public:
  void add(JobNode job);
  void addDependency(JobId job, JobId dependsOn);
  const std::unordered_map<JobId, JobNode>& jobs() const { return jobs_; }
  std::vector<JobId> topologicalOrder() const;
  std::unordered_map<JobId, std::chrono::milliseconds> criticalPathWeights() const;
 private:
  std::unordered_map<JobId, JobNode> jobs_;
};

class Scheduler {
 public:
  static std::vector<WorkerAssignment> planLpt(const std::vector<JobNode>& independentJobs, std::size_t workerCount);
  static std::vector<WorkerAssignment> planDag(const JobGraph& graph, std::size_t workerCount);
};
} // namespace testgraph
