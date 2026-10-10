#include "testgraph/parser/parser.hpp"
#include "testgraph/util.hpp"

#include <regex>
#include <sstream>

namespace testgraph {
bool CppParser::supports(const std::filesystem::path& path) const {
  const auto ext = path.extension().string();
  return ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c" || ext == ".h" || ext == ".hh" || ext == ".hpp" || ext == ".hxx";
}

ParseResult CppParser::parse(const std::filesystem::path&, std::string_view source) const {
  ParseResult result;
  static const std::regex includeRe(R"(^\s*#\s*include\s*([<"])([^>"]+)[>"])");
  std::istringstream in{std::string(source)};
  std::string line;
  while (std::getline(in, line)) {
    std::smatch m;
    if (std::regex_search(line, m, includeRe)) result.dependencies.push_back((m[1].str() == "\"" ? "@local:" : "@angle:") + m[2].str());
  }
  return result;
}

bool TypeScriptParser::supports(const std::filesystem::path& path) const {
  const auto ext = path.extension().string();
  return ext == ".ts" || ext == ".tsx" || ext == ".js" || ext == ".jsx" || ext == ".mjs" || ext == ".cjs";
}

ParseResult TypeScriptParser::parse(const std::filesystem::path&, std::string_view source) const {
  ParseResult result;
  const std::string s(source);
  const std::vector<std::regex> patterns = {
    std::regex(R"((?:import|export)[^'"\n]*['"]([^'"]+)['"])"),
    std::regex(R"(require\s*\(\s*['"]([^'"]+)['"]\s*\))"),
    std::regex(R"(import\s*\(\s*['"]([^'"]+)['"]\s*\))")
  };
  for (const auto& re : patterns) {
    for (std::sregex_iterator it(s.begin(), s.end(), re), end; it != end; ++it) result.dependencies.push_back((*it)[1].str());
  }
  static const std::regex dynamicImport(R"(import\s*\((?!\s*['"])[^)]+\))");
  if (std::regex_search(s, dynamicImport)) result.unresolved.push_back("dynamic import expression");
  return result;
}

bool PythonParser::supports(const std::filesystem::path& path) const { return path.extension() == ".py"; }

ParseResult PythonParser::parse(const std::filesystem::path&, std::string_view source) const {
  ParseResult result;
  std::istringstream in{std::string(source)};
  std::string line;
  static const std::regex importRe(R"(^\s*import\s+([A-Za-z_][A-Za-z0-9_\.]*))");
  static const std::regex fromRe(R"(^\s*from\s+([\.A-Za-z_][A-Za-z0-9_\.]*)\s+import\s+)");
  while (std::getline(in, line)) {
    std::smatch m;
    if (std::regex_search(line, m, fromRe) || std::regex_search(line, m, importRe)) result.dependencies.push_back(m[1].str());
  }
  return result;
}

ParserRegistry::ParserRegistry() {
  add(std::make_unique<CppParser>());
  add(std::make_unique<TypeScriptParser>());
  add(std::make_unique<PythonParser>());
}

void ParserRegistry::add(std::unique_ptr<ISourceParser> parser) { parsers_.push_back(std::move(parser)); }

const ISourceParser* ParserRegistry::find(const std::filesystem::path& path) const {
  for (const auto& p : parsers_) if (p->supports(path)) return p.get();
  return nullptr;
}

} // namespace testgraph
