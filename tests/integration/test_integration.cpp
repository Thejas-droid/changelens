#include "testgraph/config/config.hpp"
#include "testgraph/git/git_client.hpp"
#include "testgraph/impact/impact_analyzer.hpp"
#include "testgraph/repository/repository_analyzer.hpp"
#include "testgraph/util.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace testgraph;
namespace {
void command(const std::filesystem::path& cwd, const std::string& c) {
  const auto full = "cd '" + cwd.string() + "' && " + c;
  if (std::system(full.c_str()) != 0) throw std::runtime_error("command failed: " + c);
}
}

int main() {
  const auto root = std::filesystem::temp_directory_path() / "testgraph-integration-fixture";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root / "src");
  std::filesystem::create_directories(root / "tests");
  util::writeFile(root / "src/payment.hpp", "#pragma once\nint total();\n");
  util::writeFile(root / "src/payment.cpp", "#include \"payment.hpp\"\nint total(){return 1;}\n");
  util::writeFile(root / "tests/payment_test.cpp", "#include \"payment.hpp\"\nTEST(Payment, Total) {}\n");
  util::writeFile(root / "tests/unrelated_test.cpp", "TEST(Other, Works) {}\n");
  util::writeFile(root / "testgraph.yml", "tests:\n  patterns:\n    - \"**/*_test.cpp\"\nfallback:\n  fullSuiteWhenChanged:\n    - \"CMakeLists.txt\"\n");

  auto cfg = Config::load(root / "testgraph.yml");
  auto analysis = RepositoryAnalyzer(cfg).analyze(root);
  if (analysis.tests.size() != 2) { std::cerr << "expected 2 tests, got " << analysis.tests.size() << '\n'; return 1; }
  auto impact = ImpactAnalyzer(cfg).analyze(analysis.graph, analysis.tests, {"src/payment.hpp"}, analysis.confidence);
  if (impact.selectedTests.size() != 1) { std::cerr << "expected one selected test, got " << impact.selectedTests.size() << '\n'; return 1; }

  command(root, "git init -q && git config user.email a@b.c && git config user.name test && git add . && git commit -qm base");
  const auto base = GitClient(root).revParse("HEAD");
  util::writeFile(root / "src/payment.hpp", "#pragma once\nint total();\nint refund();\n");
  command(root, "git add . && git commit -qm change");
  const auto head = GitClient(root).revParse("HEAD");
  auto changes = GitClient(root).diff(base, head);
  if (changes.size() != 1 || changes[0].path != std::filesystem::path("src/payment.hpp")) { std::cerr << "git diff mapping failed\n"; return 1; }

  RepositoryAnalyzer analyzer(cfg);
  analyzer.updateIncremental(root, changes, analysis);
  auto impact2 = ImpactAnalyzer(cfg).analyze(analysis.graph, analysis.tests, {"src/payment.hpp"}, analysis.confidence);
  if (impact2.selectedTests.size() != 1) { std::cerr << "incremental analysis failed\n"; return 1; }
  std::filesystem::remove_all(root);
  std::cout << "integration test passed\n";
  return 0;
}
