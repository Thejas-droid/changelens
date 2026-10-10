#pragma once
#include "testgraph/domain.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
namespace testgraph {
struct AuditMiss { TestId testId{}; std::string failureSignature; };
struct AuditResult { std::vector<AuditMiss> misses; bool passed() const { return misses.empty(); } };
class AuditEvaluator {
 public:
  static AuditResult compare(const std::vector<TestId>& selected, const std::unordered_map<TestId, TestStatus>& fullSuiteStatuses, const std::unordered_map<TestId, std::string>& failureSignatures = {});
};
struct AuditStatistics { std::uint64_t auditedRuns{0}; std::uint64_t missedTests{0}; void record(const AuditResult& r){++auditedRuns; missedTests+=r.misses.size();} double accuracy() const { return auditedRuns==0?1.0:1.0-static_cast<double>(missedTests)/auditedRuns; } };
}
