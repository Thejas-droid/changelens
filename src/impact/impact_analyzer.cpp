#include "testgraph/impact/impact_analyzer.hpp"
#include "testgraph/util.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace testgraph {
ImpactResult ImpactAnalyzer::analyze(const DependencyGraph& graph, const std::vector<TestCase>& tests,
                                     const std::vector<std::filesystem::path>& changedPaths,
                                     Confidence parserConfidence) const {
  ImpactResult out;
  out.confidence = parserConfidence;
  bool fallback = parserConfidence == Confidence::Unsafe;
  std::unordered_set<FileId> changedSet;
  for (const auto& path : changedPaths) {
    const auto normalized = util::normalizePath(path);
    if (util::matchesAny(config_.fallbackPatterns, normalized)) {
      fallback = true;
      out.reasons.push_back("fallback pattern changed: " + normalized);
    }
    if (auto id = graph.findByPath(path)) {
      out.changed.push_back(*id);
      changedSet.insert(*id);
    } else {
      out.confidence = Confidence::Low;
      out.reasons.push_back("changed file not present in dependency graph: " + normalized);
    }
  }
  if (out.changed.empty() && !changedPaths.empty()) {
    fallback = true;
    out.reasons.push_back("no changed files could be mapped to graph nodes");
  }

  if (fallback) {
    out.fullSuiteFallback = true;
    for (const auto& [id, _] : graph.nodes()) out.affected.push_back(id);
    for (const auto& t : tests) out.selectedTests.push_back(t.id);
    if (out.confidence != Confidence::Unsafe) out.confidence = Confidence::Low;
    return out;
  }

  std::unordered_map<FileId, FileId> parent;
  out.affected = graph.transitiveDependents(out.changed, &parent);
  std::unordered_set<FileId> affectedSet(out.affected.begin(), out.affected.end());
  for (const auto& t : tests) {
    const bool selected = affectedSet.contains(t.fileId) || util::matchesAny(config_.alwaysRunPatterns, t.filePath.generic_string());
    if (!selected) continue;
    out.selectedTests.push_back(t.id);
    auto path = graph.explanationPath(t.fileId, changedSet);
    if (path.empty() && util::matchesAny(config_.alwaysRunPatterns, t.filePath.generic_string())) path = {t.fileId};
    out.explanations.push_back({t.id, std::move(path)});
  }
  return out;
}
} // namespace testgraph
