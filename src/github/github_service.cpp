#include "testgraph/github/github_service.hpp"
#include <iomanip>
#include <sstream>

namespace testgraph {
namespace {
std::string duration(std::chrono::milliseconds ms) {
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(ms).count();
  std::ostringstream o; o << sec / 60 << "m " << std::setw(2) << std::setfill('0') << sec % 60 << "s"; return o.str();
}
}
std::string GitHubService::formatCheckSummary(const CheckSummary& s) {
  const auto saved = s.fullSuiteEstimate > s.wallTime ? s.fullSuiteEstimate - s.wallTime : std::chrono::milliseconds{0};
  std::ostringstream o;
  o << "ChangeLens\n\nStatus: " << s.status << "\n\n"
    << "Changed files:       " << s.changedFiles << '\n'
    << "Affected files:      " << s.affectedFiles << '\n'
    << "Repository tests:    " << s.repositoryTests << '\n'
    << "Affected tests:      " << s.affectedTests << '\n'
    << "Cache hits:          " << s.cacheHits << '\n'
    << "Executed tests:      " << s.executedTests << "\n\n"
    << "Passed:              " << s.passed << '\n'
    << "Probable flaky:      " << s.probableFlaky << '\n'
    << "Failed:              " << s.failed << "\n\n"
    << "Estimated full-suite: " << duration(s.fullSuiteEstimate) << '\n'
    << "Actual wall time:     " << duration(s.wallTime) << '\n'
    << "Estimated time saved: " << duration(saved) << '\n';
  return o.str();
}
} // namespace testgraph
