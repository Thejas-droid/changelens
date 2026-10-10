#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace testgraph {
struct BuildTarget {
  std::string name;
  std::string type;
  std::vector<std::filesystem::path> sources;
  std::vector<std::string> dependencies;
  bool testTarget{false};
};
class BuildGraph {
 public:
  void upsert(BuildTarget target);
  const BuildTarget* target(std::string_view name) const;
  std::vector<std::string> targetsForFile(const std::filesystem::path& file) const;
  std::vector<std::string> affectedTargets(const std::vector<std::filesystem::path>& changedFiles) const;
  std::vector<std::string> affectedTestTargets(const std::vector<std::filesystem::path>& changedFiles) const;
  const std::unordered_map<std::string,BuildTarget>& targets() const{return targets_;}
 private:
  std::unordered_map<std::string,BuildTarget> targets_;
};
class CMakeBuildScanner {
 public:
  BuildGraph scan(const std::filesystem::path& repositoryRoot) const;
};
}
