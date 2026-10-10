#include "testgraph/repository/repository_analyzer.hpp"
#include "testgraph/test/test_discovery.hpp"
#include "testgraph/util.hpp"

#include <algorithm>
#include <unordered_set>

namespace testgraph {
RepositoryAnalyzer::RepositoryAnalyzer(Config config) : config_(std::move(config)) {}

FileKind RepositoryAnalyzer::classify(const std::filesystem::path& rel) const {
  const auto s = rel.generic_string();
  if (util::matchesAny(config_.testPatterns, s)) return FileKind::Test;
  if (rel.filename() == "CMakeLists.txt" || rel.extension() == ".cmake" || util::startsWith(s, "cmake/")) return FileKind::Build;
  if (rel.filename() == "package.json" || rel.filename() == "requirements.txt" || rel.filename() == "pyproject.toml" || rel.extension() == ".yml" || rel.extension() == ".yaml") return FileKind::Config;
  const auto ext = rel.extension().string();
  if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c" || ext == ".h" || ext == ".hh" || ext == ".hpp" || ext == ".hxx" || ext == ".ts" || ext == ".tsx" || ext == ".js" || ext == ".jsx" || ext == ".py") return FileKind::Source;
  return FileKind::Unknown;
}

std::optional<std::filesystem::path> RepositoryAnalyzer::resolveDependency(const std::filesystem::path& root,
    const std::filesystem::path& sourceRel, std::string_view dependency, const DependencyGraph& graph) const {
  const std::string dep(dependency);
  std::vector<std::filesystem::path> candidates;
  const auto langExt = sourceRel.extension().string();
  if (langExt == ".py") {
    auto p = dep;
    std::replace(p.begin(), p.end(), '.', '/');
    candidates = {p + ".py", std::filesystem::path(p) / "__init__.py"};
  } else if (langExt == ".ts" || langExt == ".tsx" || langExt == ".js" || langExt == ".jsx") {
    if (!dep.empty() && dep[0] == '.') {
      auto base = (sourceRel.parent_path() / dep).lexically_normal();
      candidates = {base, base.string()+".ts", base.string()+".tsx", base.string()+".js", base.string()+".jsx", base / "index.ts", base / "index.js"};
    } else {
      return std::nullopt; // package dependency, not repository file by default
    }
  } else {
    auto local = (sourceRel.parent_path() / dep).lexically_normal();
    candidates = {local, std::filesystem::path(dep)};
    for (const auto& r : config_.roots) candidates.push_back((r / dep).lexically_normal());
    candidates.push_back((std::filesystem::path("include") / dep).lexically_normal());
    candidates.push_back((std::filesystem::path("src") / dep).lexically_normal());
  }
  for (auto c : candidates) {
    c = c.lexically_normal();
    if (c.is_absolute() || std::find(c.begin(), c.end(), "..") != c.end()) continue;
    if (graph.findByPath(c)) return c;
    if (std::filesystem::exists(root / c)) return c;
  }
  return std::nullopt;
}

void RepositoryAnalyzer::parseAndConnect(const std::filesystem::path& root, FileId id, RepositoryAnalysis& analysis) const {
  auto* node = analysis.graph.node(id);
  if (!node) return;
  analysis.graph.clearOutgoing(id);
  const auto* parser = registry_.find(node->path);
  if (!parser) return;
  std::string source;
  try { source = util::readFile(root / node->path); } catch (...) { analysis.confidence = Confidence::Low; return; }
  const auto parsed = parser->parse(node->path, source);
  for (const auto& rawDep : parsed.dependencies) {
    std::string dep = rawDep;
    bool requiredLocal = false;
    if (parser->language() == "cpp") {
      if (util::startsWith(dep, "@local:")) { requiredLocal = true; dep = dep.substr(7); }
      else if (util::startsWith(dep, "@angle:")) dep = dep.substr(7);
    }
    auto resolved = resolveDependency(root, node->path, dep, analysis.graph);
    if (resolved) {
      if (auto target = analysis.graph.findByPath(*resolved)) {
        analysis.graph.addEdge(id, *target, parser->language() == "cpp" ? DependencyType::Include : DependencyType::Import);
      }
    } else if (requiredLocal || (!dep.empty() && dep[0] == '.')) {
      analysis.unresolved.push_back(node->path.generic_string() + ": " + dep);
      if (analysis.confidence == Confidence::High) analysis.confidence = Confidence::Medium;
    }
  }
  for (const auto& issue : parsed.unresolved) {
    analysis.unresolved.push_back(node->path.generic_string() + ": " + issue);
    if (analysis.confidence == Confidence::High) analysis.confidence = Confidence::Medium;
  }
}

RepositoryAnalysis RepositoryAnalyzer::analyze(const std::filesystem::path& root) const {
  RepositoryAnalysis result;
  if (!std::filesystem::exists(root)) throw std::runtime_error("repository root does not exist: " + root.string());
  for (auto it = std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied);
       it != std::filesystem::recursive_directory_iterator(); ++it) {
    if (it->is_symlink()) { it.disable_recursion_pending(); continue; }
    if (it->is_directory()) {
      auto rel = std::filesystem::relative(it->path(), root).generic_string();
      if (util::matchesAny(config_.ignorePatterns, rel) || util::matchesAny(config_.ignorePatterns, rel + "/x")) it.disable_recursion_pending();
      continue;
    }
    if (!it->is_regular_file()) continue;
    auto rel = std::filesystem::relative(it->path(), root).lexically_normal();
    if (util::matchesAny(config_.ignorePatterns, rel.generic_string())) continue;
    const auto* parser = registry_.find(rel);
    const auto kind = classify(rel);
    if (!parser && kind == FileKind::Unknown) continue;
    std::string content;
    try { content = util::readFile(it->path()); } catch (...) { continue; }
    const std::string lang = parser ? parser->language() : (kind == FileKind::Build ? "cmake" : "config");
    result.graph.upsertNode(rel, kind, lang, util::sha256(content));
    ++result.filesScanned;
    if (kind == FileKind::Test) ++result.testFiles;
    else if (kind == FileKind::Source) ++result.sourceFiles;
  }
  std::vector<FileId> ids;
  ids.reserve(result.graph.nodes().size());
  for (const auto& [id, _] : result.graph.nodes()) ids.push_back(id);
  for (auto id : ids) parseAndConnect(root, id, result);
  auto fingerprints = result.graph.computeDependencyFingerprints();
  for (const auto& [id, fp] : fingerprints) if (auto* n = result.graph.node(id)) n->dependencyFingerprint = fp;
  result.tests = TestDiscovery(config_).discover(root, result.graph);
  return result;
}

void RepositoryAnalyzer::updateIncremental(const std::filesystem::path& root, const std::vector<ChangedFile>& changes,
                                           RepositoryAnalysis& analysis) const {
  std::vector<FileId> reparsed;
  for (const auto& c : changes) {
    if (c.status == ChangedFile::Status::Deleted) {
      if (auto id = analysis.graph.findByPath(c.path)) analysis.graph.removeNode(*id);
      continue;
    }
    if (c.status == ChangedFile::Status::Renamed && !c.oldPath.empty()) {
      if (auto old = analysis.graph.findByPath(c.oldPath)) analysis.graph.removeNode(*old);
    }
    const auto abs = root / c.path;
    if (!std::filesystem::exists(abs) || !std::filesystem::is_regular_file(abs)) continue;
    const auto* parser = registry_.find(c.path);
    const auto kind = classify(c.path);
    if (!parser && kind == FileKind::Unknown) continue;
    const auto content = util::readFile(abs);
    const auto id = analysis.graph.upsertNode(c.path, kind, parser ? parser->language() : "config", util::sha256(content));
    reparsed.push_back(id);
  }
  analysis.unresolved.clear();
  analysis.confidence = Confidence::High;
  for (auto id : reparsed) parseAndConnect(root, id, analysis);
  auto fingerprints = analysis.graph.computeDependencyFingerprints();
  for (const auto& [id, fp] : fingerprints) if (auto* n = analysis.graph.node(id)) n->dependencyFingerprint = fp;
  analysis.tests = TestDiscovery(config_).discover(root, analysis.graph);
}
} // namespace testgraph
