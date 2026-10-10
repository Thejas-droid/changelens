#include "testgraph/metrics/metrics.hpp"
#include <sstream>

namespace testgraph {
void MetricsRegistry::increment(const std::string& name, std::uint64_t amount) {
  std::lock_guard lock(mutex_); counters_[name] += amount;
}
void MetricsRegistry::setGauge(const std::string& name, double value) {
  std::lock_guard lock(mutex_); gauges_[name] = value;
}
std::string MetricsRegistry::prometheus() const {
  std::lock_guard lock(mutex_);
  std::ostringstream o;
  for (const auto& [k, v] : counters_) o << k << ' ' << v << '\n';
  for (const auto& [k, v] : gauges_) o << k << ' ' << v << '\n';
  return o.str();
}
} // namespace testgraph
