#pragma once

#include "testgraph/domain.hpp"
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace testgraph {

class DependencyGraph {
 public:
  FileId upsertNode(const std::filesystem::path& path, FileKind kind, std::string language, std::string contentHash);
  bool removeNode(FileId id);
  void clearOutgoing(FileId id);
  bool addEdge(FileId from, FileId to, DependencyType type);
  bool removeEdge(FileId from, FileId to, DependencyType type);

  std::optional<FileId> findByPath(const std::filesystem::path& path) const;
  const FileNode* node(FileId id) const;
  FileNode* node(FileId id);
  const std::unordered_map<FileId, FileNode>& nodes() const { return nodes_; }
  const std::vector<DependencyEdge>& edges() const { return edges_; }
  const std::vector<FileId>& dependencies(FileId id) const;
  const std::vector<FileId>& dependents(FileId id) const;

  std::vector<FileId> transitiveDependents(const std::vector<FileId>& changed,
                                           std::unordered_map<FileId, FileId>* parent = nullptr) const;
  std::vector<std::vector<FileId>> stronglyConnectedComponents() const;
  std::vector<FileId> explanationPath(FileId selected, const std::unordered_set<FileId>& changed) const;
  std::unordered_map<FileId, std::size_t> transitiveDependentCounts() const;
  std::unordered_map<FileId, std::string> computeDependencyFingerprints() const;

  bool validate(std::string* reason = nullptr) const;

 private:
  FileId nextId_{1};
  std::unordered_map<FileId, FileNode> nodes_;
  std::unordered_map<std::string, FileId> pathToId_;
  std::vector<DependencyEdge> edges_;
  std::unordered_map<FileId, std::vector<FileId>> dependencies_;
  std::unordered_map<FileId, std::vector<FileId>> reverse_;
};

} // namespace testgraph
