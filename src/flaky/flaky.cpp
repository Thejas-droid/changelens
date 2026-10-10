#include "testgraph/flaky/flaky.hpp"
#include "testgraph/util.hpp"

#include <regex>

namespace testgraph {
std::string FailureSignature::normalize(std::string_view text) {
  std::string s(text);
  s = std::regex_replace(s, std::regex(R"(\b\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?Z?\b)"), "<timestamp>");
  s = std::regex_replace(s, std::regex(R"(/tmp/[A-Za-z0-9_./-]+)"), "<tmp-path>");
  s = std::regex_replace(s, std::regex(R"(<tmp-path>:\d+(?::\d+)?)"), "<tmp-path>:<line>");
  s = std::regex_replace(s, std::regex(R"(([^\s:]+\.(?:cpp|cc|cxx|h|hpp|py|ts|js)):\d+(?::\d+)?)"), "$1:<line>");
  s = std::regex_replace(s, std::regex(R"(\r\n?)"), "\n");
  s = std::regex_replace(s, std::regex(R"([ \t]+)"), " ");
  return util::trim(s);
}

std::string FailureSignature::compute(std::string_view testIdentity, std::string_view failureText) {
  return util::sha256(std::string(testIdentity) + "\n" + normalize(failureText));
}
} // namespace testgraph
