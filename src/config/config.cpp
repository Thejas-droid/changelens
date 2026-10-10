#include "testgraph/config/config.hpp"
#include "testgraph/util.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace testgraph {
namespace {
std::string unquote(std::string s) {
  s = util::trim(std::move(s));
  if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

bool parseBool(const std::string& value) {
  auto s = value;
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s == "true" || s == "yes" || s == "1" || s == "on";
}
} // namespace

Config Config::defaults() { return {}; }

Config Config::load(const std::filesystem::path& path) {
  Config cfg;
  std::ifstream in(path);
  if (!in) throw std::runtime_error("unable to read config: " + path.string());
  struct Section { int indent; std::string key; };
  std::vector<Section> stack;
  std::string line;

  auto sectionPath = [&]() {
    std::string full;
    for (const auto& s : stack) { if (!full.empty()) full += "."; full += s.key; }
    return full;
  };
  auto clearList = [&](const std::string& full) {
    if (full == "languages") cfg.languages.clear();
    else if (full == "tests.patterns") cfg.testPatterns.clear();
    else if (full == "tests.alwaysRun") cfg.alwaysRunPatterns.clear();
    else if (full == "tests.frameworks") cfg.testFrameworks.clear();
    else if (full == "fallback.fullSuiteWhenChanged") cfg.fallbackPatterns.clear();
    else if (full == "ignore") cfg.ignorePatterns.clear();
    else if (full == "generated.patterns") cfg.generatedPatterns.clear();
    else if (full == "project.roots") cfg.roots.clear();
  };
  auto addListItem = [&](const std::string& full, const std::string& item) {
    if (full == "languages") cfg.languages.push_back(item);
    else if (full == "tests.patterns") cfg.testPatterns.push_back(item);
    else if (full == "tests.alwaysRun") cfg.alwaysRunPatterns.push_back(item);
    else if (full == "tests.frameworks") cfg.testFrameworks.push_back(item);
    else if (full == "fallback.fullSuiteWhenChanged") cfg.fallbackPatterns.push_back(item);
    else if (full == "ignore") cfg.ignorePatterns.push_back(item);
    else if (full == "generated.patterns") cfg.generatedPatterns.push_back(item);
    else if (full == "project.roots") cfg.roots.emplace_back(item);
  };

  while (std::getline(in, line)) {
    const auto hash = line.find('#');
    if (hash != std::string::npos) line.erase(hash);
    if (util::trim(line).empty()) continue;
    int indent = 0;
    while (indent < static_cast<int>(line.size()) && line[indent] == ' ') ++indent;
    auto t = util::trim(line);

    if (util::startsWith(t, "- ")) {
      addListItem(sectionPath(), unquote(t.substr(2)));
      continue;
    }

    while (!stack.empty() && stack.back().indent >= indent) stack.pop_back();
    const auto colon = t.find(':');
    if (colon == std::string::npos) continue;
    const auto key = util::trim(t.substr(0, colon));
    const auto value = unquote(t.substr(colon + 1));
    std::string full = sectionPath();
    if (!full.empty()) full += ".";
    full += key;

    if (value.empty()) {
      clearList(full);
      stack.push_back({indent, key});
      continue;
    }

    if (full == "project.name") cfg.projectName = value;
    else if (full == "execution.workers") cfg.workers = std::stoi(value);
    else if (full == "execution.timeoutSeconds") cfg.timeoutSeconds = std::stoi(value);
    else if (full == "execution.maxConcurrentRunsPerRepository") cfg.maxConcurrentRunsPerRepository = std::stoi(value);
    else if (full == "cache.enabled") cfg.cacheEnabled = parseBool(value);
    else if (full == "cache.ttlSeconds") cfg.cacheTtlSeconds = std::stoi(value);
    else if (full == "flaky.enabled") cfg.flakyEnabled = parseBool(value);
    else if (full == "flaky.maxRetries") cfg.maxRetries = std::stoi(value);
    else if (full == "flaky.quarantine.enabled") cfg.quarantineEnabled = parseBool(value);
    else if (full == "validation.auditRate") cfg.auditRate = std::stod(value);
  }
  if (cfg.workers <= 0) throw std::runtime_error("execution.workers must be > 0");
  if (cfg.auditRate < 0.0 || cfg.auditRate > 1.0) throw std::runtime_error("validation.auditRate must be in [0,1]");
  return cfg;
}

std::string Config::toYaml() const {
  std::ostringstream o;
  o << "version: 1\nproject:\n  name: " << projectName << "\n  roots:\n";
  for (const auto& r : roots) o << "    - \"" << r.generic_string() << "\"\n";
  o << "languages:\n"; for (const auto& x : languages) o << "  - " << x << "\n";
  o << "tests:\n  frameworks:\n"; for (const auto& x : testFrameworks) o << "    - " << x << "\n";
  o << "  patterns:\n"; for (const auto& x : testPatterns) o << "    - \"" << x << "\"\n";
  o << "  alwaysRun:\n"; for (const auto& x : alwaysRunPatterns) o << "    - \"" << x << "\"\n";
  o << "execution:\n  workers: " << workers << "\n  timeoutSeconds: " << timeoutSeconds << "\n";
  o << "  maxConcurrentRunsPerRepository: " << maxConcurrentRunsPerRepository << "\n";
  o << "cache:\n  enabled: " << (cacheEnabled ? "true" : "false") << "\n  ttlSeconds: " << cacheTtlSeconds << "\n";
  o << "flaky:\n  enabled: " << (flakyEnabled ? "true" : "false") << "\n  maxRetries: " << maxRetries << "\n";
  o << "  quarantine:\n    enabled: " << (quarantineEnabled ? "true" : "false") << "\n";
  o << "fallback:\n  fullSuiteWhenChanged:\n"; for (const auto& x : fallbackPatterns) o << "    - \"" << x << "\"\n";
  o << "validation:\n  auditRate: " << auditRate << "\nignore:\n"; for (const auto& x : ignorePatterns) o << "  - \"" << x << "\"\n";
  return o.str();
}

} // namespace testgraph
