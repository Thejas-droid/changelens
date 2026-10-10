#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace testgraph {

struct Config {
  std::string projectName{"testgraph-project"};
  std::vector<std::filesystem::path> roots{"."};
  std::vector<std::string> languages{"cpp", "typescript", "python"};
  std::vector<std::string> testFrameworks{"googletest", "ctest", "vitest", "pytest"};
  std::vector<std::string> testPatterns{"**/*_test.cpp", "**/*_test.cc", "**/*.test.ts", "**/*.spec.ts", "**/test_*.py"};
  std::vector<std::string> alwaysRunPatterns;
  std::vector<std::string> fallbackPatterns{"CMakeLists.txt", "cmake/**", "package-lock.json", "pnpm-lock.yaml", "requirements.txt", "pyproject.toml", ".github/workflows/**", "testgraph.yml"};
  std::vector<std::string> ignorePatterns{"build/**", "dist/**", "node_modules/**", ".git/**", "**/node_modules/**", "**/dist/**", "**/.git/**"};
  std::vector<std::string> generatedPatterns;
  int workers{4};
  int timeoutSeconds{900};
  int maxConcurrentRunsPerRepository{2};
  bool cacheEnabled{true};
  int cacheTtlSeconds{604800};
  bool flakyEnabled{true};
  int maxRetries{2};
  bool quarantineEnabled{false};
  double auditRate{0.05};

  static Config load(const std::filesystem::path& path);
  static Config defaults();
  std::string toYaml() const;
};

} // namespace testgraph
