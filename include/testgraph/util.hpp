#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace testgraph::util {
inline std::string readFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open file: " + path.string());
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline void writeFile(const std::filesystem::path& path, std::string_view data) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("cannot write file: " + path.string());
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

inline std::string sha256(std::string_view input) {
  std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
  SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest.data());
  std::ostringstream os;
  os << std::hex << std::setfill('0');
  for (auto byte : digest) os << std::setw(2) << static_cast<int>(byte);
  return os.str();
}

inline std::string trim(std::string s) {
  auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
  s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
  return s;
}

inline bool startsWith(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

inline bool endsWith(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

inline std::string normalizePath(const std::filesystem::path& p) {
  return p.lexically_normal().generic_string();
}

inline bool globMatch(std::string_view pattern, std::string_view text) {
  const std::size_t n = pattern.size(), m = text.size();
  std::vector<std::vector<char>> dp(n + 1, std::vector<char>(m + 1, 0));
  dp[0][0] = 1;
  for (std::size_t i = 1; i <= n; ++i) {
    if (pattern[i - 1] == '*') dp[i][0] = dp[i - 1][0];
  }
  for (std::size_t i = 1; i <= n; ++i) {
    for (std::size_t j = 1; j <= m; ++j) {
      const char p = pattern[i - 1];
      const char t = text[j - 1];
      if (p == '*') {
        const bool doublestar = i >= 2 && pattern[i - 2] == '*';
        dp[i][j] = dp[i - 1][j] || (dp[i][j - 1] && (doublestar || t != '/'));
      } else if (p == '?' && t != '/') {
        dp[i][j] = dp[i - 1][j - 1];
      } else if (p == t) {
        dp[i][j] = dp[i - 1][j - 1];
      }
    }
  }
  return dp[n][m] != 0;
}

inline bool matchesAny(const std::vector<std::string>& patterns, std::string_view text) {
  return std::any_of(patterns.begin(), patterns.end(), [&](const auto& p) { return globMatch(p, text); });
}
} // namespace testgraph::util
