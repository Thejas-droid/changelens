#include "testgraph/audit/audit.hpp"
#include <unordered_set>
namespace testgraph {
AuditResult AuditEvaluator::compare(const std::vector<TestId>& selected, const std::unordered_map<TestId, TestStatus>& full, const std::unordered_map<TestId, std::string>& sigs) {
  std::unordered_set<TestId> chosen(selected.begin(), selected.end()); AuditResult out;
  for(const auto& [id,status]:full) if(!chosen.contains(id) && (status==TestStatus::Fail || status==TestStatus::Error)) { auto it=sigs.find(id); out.misses.push_back({id,it==sigs.end()?std::string{}:it->second}); }
  return out;
}
}
