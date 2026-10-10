#include "testgraph/cache/cache.hpp"
#include "testgraph/util.hpp"

#include <sstream>

namespace testgraph {
std::string TestCache::key(const CacheKeyInput& i) {
  std::ostringstream material;
  material << "test=" << i.testIdentity << '\n'
           << "deps=" << i.dependencyFingerprint << '\n'
           << "cmd=" << i.testCommand << '\n'
           << "runner=" << i.testRunnerVersion << '\n'
           << "runtime=" << i.runtimeVersion << '\n'
           << "build=" << i.buildConfiguration << '\n'
           << "env=" << i.environmentFingerprint << '\n'
           << "tool=" << i.toolVersion << '\n';
  return util::sha256(material.str());
}

void TestCache::put(CacheEntry entry) { entries_[entry.key] = std::move(entry); }

std::optional<CacheEntry> TestCache::getPassing(std::string_view key, std::chrono::system_clock::time_point now) const {
  auto it = entries_.find(std::string(key));
  if (it == entries_.end()) return std::nullopt;
  const auto& e = it->second;
  if (e.status != TestStatus::Pass || now - e.createdAt > e.ttl) return std::nullopt;
  return e;
}

void TestCache::clear() { entries_.clear(); }
} // namespace testgraph
