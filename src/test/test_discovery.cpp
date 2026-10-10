#include "testgraph/test/test_discovery.hpp"
#include "testgraph/util.hpp"

#include <regex>
#include <sstream>

namespace testgraph {
bool TestDiscovery::isTestPath(const std::filesystem::path& relativePath) const {
  return util::matchesAny(config_.testPatterns, relativePath.generic_string());
}

std::vector<TestCase> TestDiscovery::discover(const std::filesystem::path& root, const DependencyGraph& graph) const {
  std::vector<TestCase> out;
  TestId next = 1;
  for (const auto& [id, node] : graph.nodes()) {
    if (node.kind != FileKind::Test && !isTestPath(node.path)) continue;
    std::string source;
    try { source = util::readFile(root / node.path); } catch (...) { continue; }
    const auto ext = node.path.extension().string();
    bool found = false;
    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
      static const std::regex gtest(R"((?:TEST|TEST_F|TEST_P)\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\))");
      for (std::sregex_iterator it(source.begin(), source.end(), gtest), end; it != end; ++it) {
        const auto suite = (*it)[1].str(), name = (*it)[2].str();
        out.push_back({next++, id, node.path, "googletest", suite, name, {node.path.generic_string(), "--gtest_filter=" + suite + "." + name}});
        found = true;
      }
    } else if (ext == ".ts" || ext == ".tsx" || ext == ".js" || ext == ".jsx") {
      static const std::regex jsTest(R"((?:it|test)\s*\(\s*['"]([^'"]+)['"])");
      for (std::sregex_iterator it(source.begin(), source.end(), jsTest), end; it != end; ++it) {
        out.push_back({next++, id, node.path, "jest/vitest", node.path.filename().string(), (*it)[1].str(), {"npx", "vitest", "run", node.path.generic_string(), "-t", (*it)[1].str()}});
        found = true;
      }
    } else if (ext == ".py") {
      static const std::regex pyTest(R"(^\s*def\s+(test_[A-Za-z0-9_]+)\s*\()", std::regex::multiline);
      for (std::sregex_iterator it(source.begin(), source.end(), pyTest), end; it != end; ++it) {
        out.push_back({next++, id, node.path, "pytest", node.path.stem().string(), (*it)[1].str(), {"pytest", node.path.generic_string() + "::" + (*it)[1].str()}});
        found = true;
      }
    }
    if (!found) {
      out.push_back({next++, id, node.path, "file", node.path.filename().string(), "all", {node.path.generic_string()}});
    }
  }
  return out;
}
} // namespace testgraph
