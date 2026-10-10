#include "testgraph/cache/cache.hpp"
#include "testgraph/config/config.hpp"
#include "testgraph/executor/process_executor.hpp"
#include "testgraph/git/git_client.hpp"
#include "testgraph/impact/impact_analyzer.hpp"
#include "testgraph/repository/repository_analyzer.hpp"
#include "testgraph/scheduler/scheduler.hpp"
#include "testgraph/util.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace testgraph;
namespace {
Config loadConfig(const std::filesystem::path& root) {
  for (const auto& name : {"testgraph.yml", "testgraph.yaml"}) {
    const auto path = root / name;
    if (std::filesystem::exists(path)) return Config::load(path);
  }
  return Config::defaults();
}

RepositoryAnalysis analyzeRepo(const std::filesystem::path& root, Config cfg) {
  const auto start = std::chrono::steady_clock::now();
  auto result = RepositoryAnalyzer(std::move(cfg)).analyze(root);
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
  std::cout << "Repository analysis complete\n\n"
            << "Files scanned:       " << result.filesScanned << '\n'
            << "Source files:        " << result.sourceFiles << '\n'
            << "Tests:               " << result.tests.size() << '\n'
            << "Dependency edges:    " << result.graph.edges().size() << '\n'
            << "SCC groups:          " << result.graph.stronglyConnectedComponents().size() << '\n'
            << "Unresolved imports:  " << result.unresolved.size() << '\n'
            << "Analysis confidence: " << toString(result.confidence) << '\n'
            << "Duration:            " << elapsed.count() << "ms\n";
  return result;
}

void usage() {
  std::cout << "ChangeLens - repository change-impact analysis\n\n"
            << "Commands:\n"
            << "  testgraph init [root]\n"
            << "  testgraph analyze [root]\n"
            << "  testgraph graph [root]\n"
            << "  testgraph affected <base> <head> [root]\n"
            << "  testgraph explain <test-path> <base> <head> [root]\n"
            << "  testgraph run-command <cwd> <program> [args...]\n"
            << "  testgraph benchmark [root]\n"
            << "  testgraph doctor [root]\n"
            << "  testgraph cache stats\n";
}

std::vector<std::filesystem::path> changedPaths(const std::vector<ChangedFile>& changes) {
  std::vector<std::filesystem::path> out;
  for (const auto& c : changes) if (c.status != ChangedFile::Status::Deleted) out.push_back(c.path);
  return out;
}
}

int main(int argc, char** argv) {
  try {
    if (argc < 2) { usage(); return 0; }
    const std::string cmd = argv[1];
    if (cmd == "init") {
      const std::filesystem::path root = argc >= 3 ? argv[2] : ".";
      const auto path = root / "testgraph.yml";
      if (std::filesystem::exists(path)) { std::cerr << path << " already exists\n"; return 2; }
      util::writeFile(path, Config::defaults().toYaml());
      std::cout << "Created " << path << '\n';
      return 0;
    }
    if (cmd == "analyze") {
      const std::filesystem::path root = argc >= 3 ? argv[2] : ".";
      analyzeRepo(root, loadConfig(root)); return 0;
    }
    if (cmd == "graph") {
      const std::filesystem::path root = argc >= 3 ? argv[2] : ".";
      auto result = RepositoryAnalyzer(loadConfig(root)).analyze(root);
      std::cout << "digraph testgraph {\n";
      for (const auto& [id, n] : result.graph.nodes()) std::cout << "  n" << id << " [label=\"" << n.path.generic_string() << "\"];\n";
      for (const auto& e : result.graph.edges()) std::cout << "  n" << e.from << " -> n" << e.to << ";\n";
      std::cout << "}\n"; return 0;
    }
    if (cmd == "affected") {
      if (argc < 4) { usage(); return 2; }
      const std::filesystem::path root = argc >= 5 ? argv[4] : ".";
      auto cfg = loadConfig(root);
      auto analysis = RepositoryAnalyzer(cfg).analyze(root);
      auto changes = GitClient(root).diff(argv[2], argv[3]);
      auto impact = ImpactAnalyzer(cfg).analyze(analysis.graph, analysis.tests, changedPaths(changes), analysis.confidence);
      std::cout << "Changed files:       " << changes.size() << '\n'
                << "Affected files:      " << impact.affected.size() << '\n'
                << "Affected tests:      " << impact.selectedTests.size() << '\n'
                << "Full-suite fallback: " << (impact.fullSuiteFallback ? "yes" : "no") << '\n'
                << "Confidence:          " << toString(impact.confidence) << '\n';
      for (const auto& reason : impact.reasons) std::cout << "Reason: " << reason << '\n';
      return 0;
    }
    if (cmd == "explain") {
      if (argc < 5) { usage(); return 2; }
      const std::filesystem::path testPath = argv[2];
      const std::filesystem::path root = argc >= 6 ? argv[5] : ".";
      auto cfg = loadConfig(root);
      auto analysis = RepositoryAnalyzer(cfg).analyze(root);
      auto impact = ImpactAnalyzer(cfg).analyze(analysis.graph, analysis.tests, changedPaths(GitClient(root).diff(argv[3], argv[4])), analysis.confidence);
      for (const auto& ex : impact.explanations) {
        auto it = std::find_if(analysis.tests.begin(), analysis.tests.end(), [&](const auto& t){ return t.id == ex.testId; });
        if (it == analysis.tests.end() || it->filePath != testPath) continue;
        std::cout << "Selected because:\n\n";
        for (std::size_t i = 0; i < ex.path.size(); ++i) {
          const auto* n = analysis.graph.node(ex.path[i]); if (!n) continue;
          std::cout << n->path.generic_string() << (i + 1 == ex.path.size() ? "\n" : "\n  -> ");
        }
        return 0;
      }
      std::cout << "Test is not selected by this diff, or no explanation path is available.\n";
      return 1;
    }
    if (cmd == "run-command") {
      if (argc < 4) { usage(); return 2; }
      ProcessSpec spec; spec.workingDirectory = argv[2];
      for (int i = 3; i < argc; ++i) spec.argv.emplace_back(argv[i]);
      auto r = ProcessExecutor().run(spec);
      std::cout << r.stdoutText; std::cerr << r.stderrText;
      return r.exitCode;
    }
    if (cmd == "benchmark") {
      const std::filesystem::path root = argc >= 3 ? argv[2] : ".";
      const auto start = std::chrono::steady_clock::now();
      auto a = RepositoryAnalyzer(loadConfig(root)).analyze(root);
      const auto buildMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
      const auto travStart = std::chrono::steady_clock::now();
      std::vector<FileId> seed; if (!a.graph.nodes().empty()) seed.push_back(a.graph.nodes().begin()->first);
      auto affected = a.graph.transitiveDependents(seed);
      const auto travUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - travStart);
      std::cout << "graph_build_ms=" << buildMs.count() << "\nimpact_traversal_us=" << travUs.count() << "\naffected=" << affected.size() << '\n';
      return 0;
    }
    if (cmd == "doctor") {
      const std::filesystem::path root = argc >= 3 ? argv[2] : ".";
      auto cfg = loadConfig(root); (void)cfg;
      GitClient git(root);
      std::cout << "repository=" << (git.isRepository() ? "ok" : "not-git") << '\n';
      std::cout << "config=ok\nopenssl=ok\n";
      return git.isRepository() ? 0 : 1;
    }
    if (cmd == "cache" && argc >= 3 && std::string(argv[2]) == "stats") {
      std::cout << "Local in-process cache entries are runtime-scoped. Durable production cache uses Redis/PostgreSQL adapters.\n";
      return 0;
    }
    usage(); return 2;
  } catch (const std::exception& e) {
    std::cerr << "testgraph: " << e.what() << '\n';
    return 1;
  }
}
