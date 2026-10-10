#include "testgraph/run/run_coordinator.hpp"
#include "testgraph/flaky/flaky.hpp"
#include "testgraph/scheduler/scheduler.hpp"

#include <algorithm>
#include <sstream>
#include <thread>

namespace testgraph {
namespace {
std::string join(const std::vector<std::string>& xs) {
  std::ostringstream o;
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i) o << '\x1f';
    o << xs[i];
  }
  return o.str();
}
}

LocalRunResult LocalRunCoordinator::run(
    const std::filesystem::path& root,
    const DependencyGraph& graph,
    const std::vector<TestCase>& tests,
    const std::vector<std::filesystem::path>& changed,
    const std::unordered_map<TestId, std::chrono::milliseconds>& estimates,
    LocalRunEnvironment env,
    const CancellationToken* cancellation) const {
  const auto start = std::chrono::steady_clock::now();
  LocalRunResult result;
  result.impact = ImpactAnalyzer(config_).analyze(graph, tests, changed, Confidence::High);

  std::unordered_map<TestId, const TestCase*> byId;
  for (const auto& t : tests) byId[t.id] = &t;

  std::vector<JobNode> jobs;
  std::unordered_map<JobId, TestId> jobToTest;
  JobId nextJob = 1;
  std::mutex resultMutex;

  for (auto tid : result.impact.selectedTests) {
    auto it = byId.find(tid);
    if (it == byId.end()) continue;
    const auto& t = *it->second;
    const auto* n = graph.node(t.fileId);
    CacheKeyInput keyIn{
        t.filePath.generic_string() + "::" + t.suiteName + "::" + t.testName,
        n ? n->dependencyFingerprint : "", join(t.command), env.testRunnerVersion,
        env.runtimeVersion, env.buildConfiguration, env.environmentFingerprint,
        "testgraph-cache-v1"};
    const auto key = TestCache::key(keyIn);
    if (config_.cacheEnabled) {
      if (auto hit = cache_.getPassing(key)) {
        result.tests.push_back({tid, TestStatus::Pass, 0, true, false, hit->duration, "", ""});
        ++result.cacheHits;
        continue;
      }
    }

    JobNode j;
    j.id = nextJob++;
    j.type = JobType::UnitTest;
    j.testId = tid;
    j.command = t.command;
    j.estimatedDuration = estimates.contains(tid) ? estimates.at(tid) : std::chrono::seconds(1);
    jobs.push_back(j);
    jobToTest[j.id] = tid;
  }

  auto plan = Scheduler::planLpt(jobs, static_cast<std::size_t>(std::max(1, config_.workers)));
  std::vector<std::vector<JobId>> workerJobs(static_cast<std::size_t>(std::max(1, config_.workers)));
  for (const auto& a : plan) workerJobs[a.workerIndex].push_back(a.jobId);

  std::unordered_map<JobId, const JobNode*> jobById;
  for (const auto& j : jobs) jobById[j.id] = &j;

  std::vector<std::thread> workers;
  for (const auto& list : workerJobs) {
    workers.emplace_back([&, list] {
      for (auto jid : list) {
        if (cancellation && cancellation->cancelled()) break;
        const auto& j = *jobById.at(jid);
        const auto& t = *byId.at(jobToTest.at(jid));
        TestRunRecord rec;
        rec.testId = t.id;

        if (j.command.empty()) {
          rec.status = TestStatus::Error;
          rec.output = "no executable command produced by test adapter";
          std::lock_guard lock(resultMutex);
          result.tests.push_back(std::move(rec));
          result.passed = false;
          continue;
        }

        std::string firstFailure;
        const int maxAttempts = 1 + (config_.flakyEnabled ? config_.maxRetries : 0);
        for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
          ProcessSpec spec;
          spec.argv = j.command;
          spec.workingDirectory = root;
          spec.timeout = std::chrono::seconds(config_.timeoutSeconds);
          auto pr = ProcessExecutor().run(spec, cancellation);
          rec.attempts = attempt;
          rec.duration += pr.duration;
          rec.output = pr.stdoutText + pr.stderrText;

          if (pr.exitCode == 0 && !pr.timedOut && !pr.cancelled) {
            rec.status = TestStatus::Pass;
            if (attempt > 1) rec.probableFlaky = true;
            break;
          }
          rec.status = (pr.timedOut || pr.cancelled) ? TestStatus::Error : TestStatus::Fail;
          if (firstFailure.empty()) firstFailure = rec.output;
          if (pr.cancelled) break;
        }

        if (rec.status != TestStatus::Pass || rec.probableFlaky) {
          rec.failureSignature = FailureSignature::compute(
              t.filePath.generic_string() + "::" + t.testName, firstFailure);
        }

        if (rec.status == TestStatus::Pass && config_.cacheEnabled) {
          const auto* n = graph.node(t.fileId);
          CacheKeyInput k{
              t.filePath.generic_string() + "::" + t.suiteName + "::" + t.testName,
              n ? n->dependencyFingerprint : "", join(t.command), env.testRunnerVersion,
              env.runtimeVersion, env.buildConfiguration, env.environmentFingerprint,
              "testgraph-cache-v1"};
          cache_.put({TestCache::key(k), TestStatus::Pass, std::chrono::system_clock::now(),
                      std::chrono::seconds(config_.cacheTtlSeconds), rec.duration});
        }

        std::lock_guard lock(resultMutex);
        if (rec.probableFlaky) ++result.flakyCount;
        if (rec.status != TestStatus::Pass) result.passed = false;
        result.tests.push_back(std::move(rec));
      }
    });
  }

  for (auto& w : workers) w.join();
  std::sort(result.tests.begin(), result.tests.end(), [](const auto& a, const auto& b) {
    return a.testId < b.testId;
  });
  result.wallTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  return result;
}

} // namespace testgraph
