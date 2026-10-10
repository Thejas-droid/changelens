#pragma once

#include "testgraph/domain.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace testgraph {
class GitClient {
 public:
  explicit GitClient(std::filesystem::path repositoryRoot) : root_(std::move(repositoryRoot)) {}
  std::vector<ChangedFile> diff(std::string_view base, std::string_view head) const;
  std::string revParse(std::string_view ref) const;
  bool isRepository() const;
 private:
  std::filesystem::path root_;
  std::string run(const std::vector<std::string>& args) const;
};
} // namespace testgraph
