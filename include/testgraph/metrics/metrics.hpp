#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace testgraph {
class MetricsRegistry {
 public:
  void increment(const std::string& name, std::uint64_t amount = 1);
  void setGauge(const std::string& name, double value);
  std::string prometheus() const;
 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::uint64_t> counters_;
  std::unordered_map<std::string, double> gauges_;
};
} // namespace testgraph
