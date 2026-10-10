#pragma once

#include "testgraph/domain.hpp"
#include <memory>
#include <string_view>
#include <vector>

namespace testgraph {

class ISourceParser {
 public:
  virtual ~ISourceParser() = default;
  virtual bool supports(const std::filesystem::path& path) const = 0;
  virtual ParseResult parse(const std::filesystem::path& path, std::string_view source) const = 0;
  virtual std::string language() const = 0;
};

class CppParser final : public ISourceParser {
 public:
  bool supports(const std::filesystem::path& path) const override;
  ParseResult parse(const std::filesystem::path&, std::string_view source) const override;
  std::string language() const override { return "cpp"; }
};

class TypeScriptParser final : public ISourceParser {
 public:
  bool supports(const std::filesystem::path& path) const override;
  ParseResult parse(const std::filesystem::path&, std::string_view source) const override;
  std::string language() const override { return "typescript"; }
};

class PythonParser final : public ISourceParser {
 public:
  bool supports(const std::filesystem::path& path) const override;
  ParseResult parse(const std::filesystem::path&, std::string_view source) const override;
  std::string language() const override { return "python"; }
};

class ParserRegistry {
 public:
  ParserRegistry();
  void add(std::unique_ptr<ISourceParser> parser);
  const ISourceParser* find(const std::filesystem::path& path) const;
 private:
  std::vector<std::unique_ptr<ISourceParser>> parsers_;
};

} // namespace testgraph
