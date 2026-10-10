#pragma once

#include "testgraph/domain.hpp"
#include <atomic>

namespace testgraph {
class CancellationToken {
 public:
  void cancel() { cancelled_.store(true, std::memory_order_relaxed); }
  bool cancelled() const { return cancelled_.load(std::memory_order_relaxed); }
 private:
  std::atomic<bool> cancelled_{false};
};

class ProcessExecutor {
 public:
  ProcessResult run(const ProcessSpec& spec, const CancellationToken* cancellation = nullptr) const;
};
} // namespace testgraph
