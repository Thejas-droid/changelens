#include "testgraph/build/build_graph.hpp"
#include "testgraph/cache/cache.hpp"
#include "testgraph/config/config.hpp"
#include "testgraph/executor/process_executor.hpp"
#include "testgraph/flaky/flaky.hpp"
#include "testgraph/graph/dependency_graph.hpp"
#include "testgraph/impact/impact_analyzer.hpp"
#include "testgraph/run/run_coordinator.hpp"
#include "testgraph/scheduler/scheduler.hpp"
#include "testgraph/security/webhook_verifier.hpp"
#include "testgraph/worker/worker_registry.hpp"
#include "testgraph/util.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace testgraph;
using namespace std::chrono_literals;
namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __FILE__ << ':' << __LINE__ << " CHECK failed: " #x "\n"; ++failures; } } while (0)
#define CHECK_EQ(a,b) CHECK((a) == (b))

void testBuildGraph() {
  const auto root = std::filesystem::temp_directory_path() / "testgraph-cmake-test";
  std::filesystem::remove_all(root); std::filesystem::create_directories(root / "src"); std::filesystem::create_directories(root / "tests");
  util::writeFile(root / "src/x.cpp", "int x(){return 1;}\n"); util::writeFile(root / "tests/x_test.cpp", "int main(){}\n");
  util::writeFile(root / "CMakeLists.txt", "add_library(core src/x.cpp)\nadd_executable(core_tests tests/x_test.cpp)\ntarget_link_libraries(core_tests PRIVATE core)\nadd_test(NAME core COMMAND core_tests)\n");
  auto g=CMakeBuildScanner().scan(root); CHECK(g.target("core")!=nullptr); CHECK(g.target("core_tests")!=nullptr); auto a=g.affectedTestTargets({"src/x.cpp"}); CHECK_EQ(a.size(),1u); CHECK_EQ(a[0],"core_tests");
  std::filesystem::remove_all(root);
}

void testGraph() {
  DependencyGraph g;
  auto a = g.upsertNode("src/a.cpp", FileKind::Source, "cpp", "a");
  auto b = g.upsertNode("src/b.hpp", FileKind::Source, "cpp", "b");
  auto c = g.upsertNode("tests/a_test.cpp", FileKind::Test, "cpp", "c");
  g.addEdge(a, b, DependencyType::Include);
  g.addEdge(c, a, DependencyType::Include);
  g.addEdge(b, a, DependencyType::Include); // cycle a<->b
  std::string reason;
  CHECK(g.validate(&reason));
  auto affected = g.transitiveDependents({b});
  CHECK_EQ(affected.size(), 3u);
  auto scc = g.stronglyConnectedComponents();
  bool foundCycle = false; for (const auto& x : scc) if (x.size() == 2) foundCycle = true;
  CHECK(foundCycle);
  auto path = g.explanationPath(c, {b});
  CHECK_EQ(path.size(), 3u);
  CHECK_EQ(path.front(), c); CHECK_EQ(path.back(), b);
  auto fp = g.computeDependencyFingerprints();
  CHECK_EQ(fp[a], fp[b]);
  CHECK(fp[a] != fp[c]);
}

void testImpact() {
  DependencyGraph g;
  auto src = g.upsertNode("src/x.hpp", FileKind::Source, "cpp", "x");
  auto test = g.upsertNode("tests/x_test.cpp", FileKind::Test, "cpp", "t");
  g.addEdge(test, src, DependencyType::Include);
  std::vector<TestCase> tests{{1, test, "tests/x_test.cpp", "googletest", "X", "Works", {}}};
  auto r = ImpactAnalyzer().analyze(g, tests, {"src/x.hpp"});
  CHECK_EQ(r.selectedTests.size(), 1u);
  CHECK(!r.fullSuiteFallback);
  auto fallback = ImpactAnalyzer().analyze(g, tests, {"CMakeLists.txt"});
  CHECK(fallback.fullSuiteFallback);
  CHECK_EQ(fallback.selectedTests.size(), 1u);
}

void testRunCoordinator() {
  DependencyGraph g;
  auto src=g.upsertNode("src/x.hpp",FileKind::Source,"cpp","src");
  auto tf=g.upsertNode("tests/x_test.cpp",FileKind::Test,"cpp","test");
  g.addEdge(tf,src,DependencyType::Include);
  auto fps=g.computeDependencyFingerprints(); for(const auto& [id,fp]:fps) g.node(id)->dependencyFingerprint=fp;
  std::vector<TestCase> tests{{1,tf,"tests/x_test.cpp","custom","X","Pass",{"/bin/true"}}};
  Config c; c.workers=2; c.maxRetries=1; c.timeoutSeconds=2; TestCache cache; LocalRunCoordinator rc(c,cache);
  auto first=rc.run(".",g,tests,{"src/x.hpp"}); CHECK(first.passed); CHECK_EQ(first.tests.size(),1u); CHECK(!first.tests[0].cacheHit); CHECK_EQ(first.tests[0].attempts,1);
  auto second=rc.run(".",g,tests,{"src/x.hpp"}); CHECK(second.passed); CHECK_EQ(second.cacheHits,1u); CHECK(second.tests[0].cacheHit);
}

void testScheduler() {
  std::vector<JobNode> jobs;
  for (int i = 0; i < 4; ++i) {
    JobNode j; j.id = i + 1; j.estimatedDuration = std::chrono::milliseconds((4-i)*100); jobs.push_back(j);
  }
  auto p = Scheduler::planLpt(jobs, 2);
  CHECK_EQ(p.size(), 4u);
  JobGraph g;
  JobNode a; a.id=1; a.estimatedDuration=100ms; JobNode b; b.id=2; b.estimatedDuration=200ms; JobNode c; c.id=3; c.estimatedDuration=50ms;
  g.add(a); g.add(b); g.add(c); g.addDependency(2,1); g.addDependency(3,2);
  auto order = g.topologicalOrder(); CHECK_EQ(order.size(), 3u); CHECK_EQ(order[0],1u);
  auto weights = g.criticalPathWeights(); CHECK_EQ(weights[1], 350ms);
}

void testCache() {
  CacheKeyInput input{"T", "D", "cmd", "runner", "runtime", "debug", "env", "tool"};
  auto k1 = TestCache::key(input); input.environmentFingerprint = "env2"; auto k2 = TestCache::key(input);
  CHECK(k1 != k2);
  TestCache cache; cache.put({k1, TestStatus::Pass, std::chrono::system_clock::now(), std::chrono::seconds(10), 10ms});
  CHECK(cache.getPassing(k1).has_value());
  cache.put({k2, TestStatus::Fail, std::chrono::system_clock::now(), std::chrono::seconds(10), 10ms});
  CHECK(!cache.getPassing(k2).has_value());
}

void testFlaky() {
  auto a = FailureSignature::compute("T", "2026-01-02T03:04:05Z /tmp/a/x.cpp:12 boom");
  auto b = FailureSignature::compute("T", "2027-02-03T04:05:06Z /tmp/b/x.cpp:88 boom");
  CHECK_EQ(a,b);
  FlakeStats s{4,3}; CHECK(s.score() > 0.74 && s.score() < 0.76);
}

void testWebhook() {
  const auto sig = WebhookVerifier::signSha256("secret", "payload");
  CHECK(WebhookVerifier::verifySha256("secret", "payload", sig));
  CHECK(!WebhookVerifier::verifySha256("secret", "tampered", sig));
}

void testWorker() {
  WorkerRegistry r; r.registerWorker("w", 4, 1000); CHECK(r.heartbeat("w", 42, 2, 500));
  std::this_thread::sleep_for(2ms);
  auto jobs = r.markDeadAndCollectJobs(1ms); CHECK_EQ(jobs.size(),1u); CHECK_EQ(jobs[0],42u);
  CompletionGate gate; CHECK(gate.accept(1,1,"tok")); CHECK(!gate.accept(1,1,"tok")); CHECK(gate.accept(1,2,"tok"));
}

void testProcess() {
  ProcessSpec spec; spec.argv = {"/bin/echo", "hello"}; spec.workingDirectory = "."; spec.timeout = 2s;
  auto r = ProcessExecutor().run(spec); CHECK_EQ(r.exitCode,0); CHECK(r.stdoutText.find("hello") != std::string::npos);
  ProcessSpec slow; slow.argv = {"/bin/sleep", "1"}; slow.timeout = 20ms;
  auto t = ProcessExecutor().run(slow); CHECK(t.timedOut); CHECK(t.exitCode != 0);
}

void testConfig() {
  auto dir = std::filesystem::temp_directory_path() / "testgraph-config-test";
  std::filesystem::create_directories(dir);
  auto path = dir / "testgraph.yml";
  util::writeFile(path, "project:\n  name: demo\n  roots:\n    - src\n    - lib\nexecution:\n  workers: 7\ncache:\n  enabled: false\nvalidation:\n  auditRate: 0.2\ntests:\n  frameworks:\n    - googletest\n  patterns:\n    - \"**/*_spec.cpp\"\n");
  auto c = Config::load(path); CHECK_EQ(c.projectName,"demo"); CHECK_EQ(c.workers,7); CHECK(!c.cacheEnabled); CHECK_EQ(c.testPatterns.size(),1u); CHECK_EQ(c.roots.size(),2u); CHECK_EQ(c.testFrameworks.size(),1u);
  std::filesystem::remove_all(dir);
}
}

int main() {
  testBuildGraph(); testGraph(); testImpact(); testRunCoordinator(); testScheduler(); testCache(); testFlaky(); testWebhook(); testWorker(); testProcess(); testConfig();
  if (failures) { std::cerr << failures << " test(s) failed\n"; return 1; }
  std::cout << "all unit tests passed\n"; return 0;
}
