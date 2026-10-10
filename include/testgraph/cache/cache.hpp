#pragma once

#include "testgraph/domain.hpp"
#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

namespace testgraph {
struct CacheKeyInput {
  std::string testIdentity;
  std::string dependencyFingerprint;
  std::string testCommand;
  std::string testRunnerVersion;
  std::string runtimeVersion;
  std::string buildConfiguration;
  std::string environmentFingerprint;
  std::string toolVersion{"testgraph-cache-v1"};
};

struct CacheEntry {
  std::string key;
  TestStatus status{TestStatus::Pass};
  std::chrono::system_clock::time_point createdAt;
  std::chrono::seconds ttl{std::chrono::hours(24 * 7)};
  std::chrono::milliseconds duration{0};
};

class TestCache {
 public:
  static std::string key(const CacheKeyInput& input);
  void put(CacheEntry entry);
  std::optional<CacheEntry> getPassing(std::string_view key, std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const;
  void clear();
  std::size_t size() const { return entries_.size(); }
 private:
  std::unordered_map<std::string, CacheEntry> entries_;
};
} // namespace testgraph
