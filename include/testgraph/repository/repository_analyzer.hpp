#pragma once

#include "testgraph/config/config.hpp"
#include "testgraph/domain.hpp"
#include "testgraph/graph/dependency_graph.hpp"
#include "testgraph/parser/parser.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace testgraph {
struct RepositoryAnalysis {
  DependencyGraph graph;
  std::vector<TestCase> tests;
  std::vector<std::string> unresolved;
  Confidence confidence{Confidence::High};
  std::size_t filesScanned{0};
  std::size_t sourceFiles{0};
  std::size_t testFiles{0};
};

class RepositoryAnalyzer {
 public:
  explicit RepositoryAnalyzer(Config config = Config::defaults());
  RepositoryAnalysis analyze(const std::filesystem::path& root) const;
  void updateIncremental(const std::filesystem::path& root,
                         const std::vector<ChangedFile>& changes,
                         RepositoryAnalysis& analysis) const;
 private:
  Config config_;
  ParserRegistry registry_;
  FileKind classify(const std::filesystem::path& relative) const;
  std::optional<std::filesystem::path> resolveDependency(const std::filesystem::path& root,
      const std::filesystem::path& sourceRel, std::string_view dependency,
      const DependencyGraph& graph) const;
  void parseAndConnect(const std::filesystem::path& root, FileId id, RepositoryAnalysis& analysis) const;
};
} // namespace testgraph
