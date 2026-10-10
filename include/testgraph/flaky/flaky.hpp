#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace testgraph {
struct FlakeStats {
  std::size_t initialFailures{0};
  std::size_t retrySuccesses{0};
  double score() const { return initialFailures == 0 ? 0.0 : static_cast<double>(retrySuccesses) / initialFailures; }
};

class FailureSignature {
 public:
  static std::string normalize(std::string_view text);
  static std::string compute(std::string_view testIdentity, std::string_view failureText);
};
} // namespace testgraph
