#include "testgraph/db/persistence.hpp"

namespace testgraph {
bool InMemoryPersistence::recordWebhookDelivery(std::string deliveryId) {
  std::lock_guard lock(mutex_);
  return deliveries_.insert(std::move(deliveryId)).second;
}
void InMemoryPersistence::saveRun(RunRecord run) {
  std::lock_guard lock(mutex_); runs_[run.id] = std::move(run);
}
std::optional<RunRecord> InMemoryPersistence::run(std::string_view id) const {
  std::lock_guard lock(mutex_);
  auto it = runs_.find(std::string(id));
  return it == runs_.end() ? std::nullopt : std::optional<RunRecord>(it->second);
}
std::vector<RunRecord> InMemoryPersistence::runs() const {
  std::lock_guard lock(mutex_);
  std::vector<RunRecord> out; out.reserve(runs_.size());
  for (const auto& [_, r] : runs_) out.push_back(r);
  return out;
}
void InMemoryPersistence::saveExecution(TestExecutionRecord record) {
  std::lock_guard lock(mutex_); executions_.push_back(std::move(record));
}
std::vector<TestExecutionRecord> InMemoryPersistence::executions(std::string_view runId) const {
  std::lock_guard lock(mutex_);
  std::vector<TestExecutionRecord> out;
  for (const auto& e : executions_) if (e.runId == runId) out.push_back(e);
  return out;
}
} // namespace testgraph
