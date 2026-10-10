#pragma once
#include "testgraph/domain.hpp"
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace testgraph {
struct CoverageObservation { std::size_t observationCount{0}; std::chrono::system_clock::time_point lastObservedAt{std::chrono::system_clock::now()}; };
class CoverageMap {
 public:
  void observe(TestId test, FileId file);
  std::vector<TestId> testsForFiles(const std::vector<FileId>& files) const;
  bool covers(TestId test, FileId file) const;
  std::size_t edgeCount() const;
 private:
  std::unordered_map<TestId, std::unordered_map<FileId, CoverageObservation>> byTest_;
  std::unordered_map<FileId, std::unordered_set<TestId>> byFile_;
};
}
