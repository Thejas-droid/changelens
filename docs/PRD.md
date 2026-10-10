# ChangeLens — Complete Product Requirements Document

> **Developer CI / Test Intelligence Engine**
>
> A production-grade, dependency-aware CI optimization platform implemented primarily in **C++**.

---

## 1. Document Status

- **Project Name:** ChangeLens
- **Project Type:** Developer Infrastructure / CI-CD / Test Intelligence
- **Primary Language:** C++20 or newer
- **Target Platform:** Linux first
- **Primary SCM Integration:** GitHub
- **Initial Repository Support:** C++, JavaScript/TypeScript, Python
- **Primary Test Integration:** adapter-based; GoogleTest, Catch2, Jest/Vitest, Pytest
- **Database:** PostgreSQL
- **Fast State / Queue:** Redis
- **Build System:** CMake
- **Containerization:** Docker
- **Frontend:** React + TypeScript
- **Status:** Full project specification
- **Scope:** Complete system, not a demo-only or MVP-only implementation

---

# 2. Product Summary

ChangeLens is an intelligent CI execution engine that determines which tests are actually affected by a code change instead of rerunning an entire test suite for every pull request.

The system:

1. receives a GitHub pull-request event,
2. fetches the repository and Git diff,
3. parses source files and dependency relationships,
4. builds and maintains a dependency graph,
5. computes the transitive impact of changed files,
6. determines affected tests,
7. calculates safe cache reuse,
8. constructs an execution DAG,
9. schedules tests across parallel workers using historical runtime estimates,
10. retries probable flaky failures,
11. stores test history and performance metrics,
12. posts a GitHub Check result,
13. exposes a dashboard for saved CI time, cache hits, flaky tests, failures, graph impact, and worker utilization.

The central product principle is:

> **Execute CI work according to change impact rather than rerunning everything indiscriminately.**

---

# 3. Why This Project Exists

Large repositories accumulate thousands of tests.

A normal pull request may modify only a few files, but traditional CI often still executes every test.

This causes:

- long developer feedback loops,
- wasted CI compute,
- queue contention,
- low worker utilization,
- repeated execution of identical work,
- difficulty identifying flaky tests,
- weak visibility into why CI is slow.

Example:

```text
Repository tests:      3,200
Changed files:             4
Actually affected tests:  93
Traditional CI:         3,200 tests
ChangeLens:                 93 candidates
Cache hits:                28
Tests executed:            65
```

ChangeLens attempts to make this optimization **safe, measurable, explainable, and reproducible**.

---

# 4. Product Goals

## 4.1 Functional Goals

ChangeLens must:

- detect files changed between two commits,
- parse repository imports and dependencies,
- construct directed dependency graphs,
- maintain reverse dependency edges,
- support cyclic dependency handling,
- identify test files and test cases,
- map tests to affected production code,
- compute transitive dependents,
- explain why every selected test was selected,
- construct an execution DAG,
- execute independent jobs concurrently,
- support local and remote workers,
- persist historical test duration,
- use historical data for test scheduling,
- calculate cache keys using code and transitive dependencies,
- reuse valid previous passing test results,
- invalidate cache safely,
- detect potentially flaky tests,
- retry failures based on configured policy,
- distinguish test failures from infrastructure failures,
- cancel stale runs when new commits arrive,
- recover jobs from dead workers,
- integrate with GitHub Apps and GitHub Checks,
- expose a CLI,
- expose a REST API,
- expose operational metrics,
- provide a dashboard,
- support correctness auditing with periodic full-suite validation.

## 4.2 Engineering Goals

The project must demonstrate:

- graph traversal,
- SCC detection,
- DAG construction,
- topological scheduling,
- concurrent worker pools,
- load balancing,
- caching,
- distributed coordination,
- idempotency,
- cancellation,
- failure recovery,
- persistent state,
- observability,
- API integration,
- CI/CD,
- secure webhook handling,
- Docker-based execution isolation.

---

# 5. Non-Goals

ChangeLens is not intended to:

- replace test frameworks,
- replace Git itself,
- replace GitHub Actions entirely,
- execute arbitrary public repositories on insecure shared hosts,
- guarantee perfect impact analysis for every dynamic language construct,
- infer business correctness from source code,
- use AI as the core test-selection mechanism.

AI may later assist with failure explanation, but the core engine must remain deterministic.

---

# 6. Target Users

## 6.1 Application Developer

Needs:

- fast PR feedback,
- understandable failures,
- confidence that relevant tests ran.

Questions:

- Why did this test run?
- Why was that test skipped?
- Which changed file caused this test to be selected?

## 6.2 Platform / DevOps Engineer

Needs:

- lower CI cost,
- faster pipelines,
- observability,
- execution history,
- flaky-test reports.

Questions:

- Which tests are consuming the most time?
- Which modules trigger the most tests?
- Which tests are unstable?
- How much compute did caching save?

## 6.3 Engineering Manager

Needs:

- CI performance trends,
- measurable reduction in feedback time,
- reliability evidence.

Questions:

- Has median PR runtime improved?
- What is the false-negative rate?
- How many engineering hours were saved?

---

# 7. Complete System Architecture

```text
                       ┌───────────────────────┐
                       │       GitHub          │
                       │ PR / Push / Checks    │
                       └──────────┬────────────┘
                                  │ webhook
                                  ▼
                    ┌─────────────────────────┐
                    │  ChangeLens API Gateway  │
                    │  C++ HTTP Service       │
                    └────────────┬────────────┘
                                 │
                  ┌──────────────┴───────────────┐
                  │                              │
                  ▼                              ▼
      ┌──────────────────────┐        ┌───────────────────────┐
      │ GitHub Integration   │        │ Run Coordinator       │
      │ App auth / Checks    │        │ lifecycle / cancel    │
      └──────────────────────┘        └───────────┬───────────┘
                                                 │
                                                 ▼
                                  ┌───────────────────────────┐
                                  │ Repository Service        │
                                  │ clone/fetch/worktree      │
                                  └──────────────┬────────────┘
                                                 │
                                                 ▼
                                  ┌───────────────────────────┐
                                  │ Static Analysis Engine    │
                                  │ parsers / imports / graph │
                                  └──────────────┬────────────┘
                                                 │
                                                 ▼
                                  ┌───────────────────────────┐
                                  │ Impact Analysis Engine    │
                                  │ reverse traversal / rules │
                                  └──────────────┬────────────┘
                                                 │
                       ┌─────────────────────────┴────────────────────┐
                       │                                              │
                       ▼                                              ▼
          ┌─────────────────────────┐                    ┌────────────────────────┐
          │ Cache Resolver          │                    │ Execution Plan Builder │
          │ dependency fingerprints │                    │ DAG + priorities       │
          └────────────┬────────────┘                    └───────────┬────────────┘
                       │                                             │
                       └──────────────────────┬──────────────────────┘
                                              ▼
                                 ┌────────────────────────────┐
                                 │ Scheduler                  │
                                 │ ready queue / LPT / DAG    │
                                 └─────────────┬──────────────┘
                                               │
                           ┌───────────────────┼───────────────────┐
                           ▼                   ▼                   ▼
                    ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
                    │ Worker A    │     │ Worker B    │     │ Worker C    │
                    │ Docker      │     │ Docker      │     │ Docker      │
                    └──────┬──────┘     └──────┬──────┘     └──────┬──────┘
                           │                   │                   │
                           └───────────────────┼───────────────────┘
                                               ▼
                                  ┌──────────────────────────┐
                                  │ Result Processor         │
                                  │ retry / flaky / metrics  │
                                  └─────────────┬────────────┘
                                                │
                         ┌──────────────────────┼──────────────────────┐
                         ▼                      ▼                      ▼
                  ┌──────────────┐       ┌──────────────┐       ┌──────────────┐
                  │ PostgreSQL   │       │ Redis        │       │ GitHub Check │
                  │ history      │       │ queues/state │       │ Reporter     │
                  └──────────────┘       └──────────────┘       └──────────────┘
                         │
                         ▼
                  ┌──────────────────────┐
                  │ React Dashboard      │
                  │ analytics / graph    │
                  └──────────────────────┘
```

---

# 8. C++ Technology Stack

## 8.1 Core Language

Use:

```text
C++20
```

or newer where available.

Required concepts:

- RAII,
- `std::filesystem`,
- `std::thread`,
- `std::jthread`,
- atomics,
- mutexes,
- condition variables,
- futures/promises,
- ranges,
- chrono,
- smart pointers,
- templates,
- coroutines where useful but not mandatory.

---

## 8.2 Build System

Use:

```text
CMake
```

Structure:

```text
cmake/
CMakeLists.txt
```

Dependencies should be managed using one consistent approach:

- vcpkg,
- Conan,
- FetchContent.

Do not mix dependency managers unnecessarily.

---

## 8.3 Recommended C++ Libraries

### HTTP Server

Recommended:

```text
Drogon
```

Alternative:

```text
Boost.Beast
```

The architecture must keep HTTP transport separate from domain logic.

### JSON

```text
nlohmann/json
```

### Git

```text
libgit2
```

### SQL

```text
libpqxx
```

### Redis

```text
redis-plus-plus
```

or a thin hiredis wrapper.

### Logging

```text
spdlog
```

### CLI

```text
CLI11
```

### Parsing

Use parser adapters.

For C/C++:

- Clang LibTooling / clang tooling.

For JavaScript / TypeScript / Python:

- Tree-sitter C API.

### Testing

```text
GoogleTest
GoogleMock
```

### Metrics

Expose Prometheus-compatible metrics through the HTTP service.

### Hashing

Use:

- SHA-256 from a vetted crypto library, or
- BLAKE3 if chosen deliberately.

Do not implement cryptographic hashing manually.

---

# 9. Repository Layout

```text
testgraph/
│
├── CMakeLists.txt
├── cmake/
│
├── README.md
├── LICENSE
├── docker-compose.yml
├── Dockerfile
├── testgraph.example.yml
│
├── apps/
│   ├── server/
│   │   └── main.cpp
│   ├── cli/
│   │   └── main.cpp
│   └── worker/
│       └── main.cpp
│
├── include/testgraph/
│   ├── api/
│   ├── cache/
│   ├── config/
│   ├── db/
│   ├── executor/
│   ├── flaky/
│   ├── git/
│   ├── github/
│   ├── graph/
│   ├── impact/
│   ├── metrics/
│   ├── parser/
│   ├── repository/
│   ├── scheduler/
│   ├── security/
│   ├── test/
│   └── worker/
│
├── src/
│   ├── api/
│   ├── cache/
│   ├── config/
│   ├── db/
│   ├── executor/
│   ├── flaky/
│   ├── git/
│   ├── github/
│   ├── graph/
│   ├── impact/
│   ├── metrics/
│   ├── parser/
│   ├── repository/
│   ├── scheduler/
│   ├── security/
│   ├── test/
│   └── worker/
│
├── migrations/
│
├── web/
│   └── dashboard/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── e2e/
│   └── fixtures/
│
├── benchmarks/
│
├── tools/
│   └── fixture-generator/
│
└── docs/
    ├── architecture.md
    ├── graph-engine.md
    ├── scheduler.md
    ├── cache.md
    ├── security.md
    ├── workers.md
    └── operations.md
```

---

# 10. Core Domain Models

## 10.1 Repository

```cpp
struct Repository {
    RepositoryId id;
    std::string githubOwner;
    std::string githubName;
    std::string defaultBranch;
    std::string installationId;
};
```

## 10.2 File Node

```cpp
enum class FileKind {
    Source,
    Test,
    Config,
    Build,
    Unknown
};

struct FileNode {
    FileId id;
    std::filesystem::path path;
    FileKind kind;
    std::string language;
    std::string contentHash;
};
```

## 10.3 Dependency Edge

```cpp
enum class DependencyType {
    Import,
    Include,
    RuntimeObserved,
    BuildDependency
};

struct DependencyEdge {
    FileId from;
    FileId to;
    DependencyType type;
};
```

## 10.4 Test Case

```cpp
struct TestCase {
    TestId id;
    FileId fileId;
    std::string framework;
    std::string suiteName;
    std::string testName;
};
```

## 10.5 Run

```cpp
enum class RunStatus {
    Queued,
    Analyzing,
    Planning,
    Running,
    Passed,
    Failed,
    Warning,
    Cancelled,
    Error
};
```

---

# 11. Git Integration

## 11.1 Repository Acquisition

The system must support:

```text
clone
fetch
checkout
worktree creation
worktree deletion
```

Use `libgit2` where practical.

Each CI run must execute in an isolated working directory.

Example:

```text
/var/lib/testgraph/runs/<run-id>/repo
```

---

## 11.2 Diff Detection

Required inputs:

```text
base SHA
head SHA
```

Calculate:

```text
added files
modified files
deleted files
renamed files
```

Renames must preserve graph identity where possible.

---

## 11.3 Changed File Classification

Classify changes:

```text
SOURCE
TEST
CONFIG
DEPENDENCY_MANIFEST
BUILD_SYSTEM
CI_CONFIGURATION
UNKNOWN
```

Certain categories may trigger full-suite fallback.

---

# 12. Source Parsing System

The parser subsystem must be plugin-based.

```cpp
class ISourceParser {
public:
    virtual ~ISourceParser() = default;

    virtual bool supports(
        const std::filesystem::path& path
    ) const = 0;

    virtual ParseResult parse(
        const std::filesystem::path& path,
        std::string_view source
    ) = 0;
};
```

---

# 13. C++ Dependency Parsing

For `.cpp`, `.cc`, `.cxx`, `.h`, `.hpp`:

Extract:

```cpp
#include "payment/service.hpp"
#include <project/auth.hpp>
```

Prefer Clang tooling for correct include resolution.

Track:

```text
translation unit
header include
project-local include
generated include
```

System headers should generally not become repository nodes.

---

# 14. JavaScript / TypeScript Parsing

Recognize:

```typescript
import x from "./x";
import { y } from "@pkg/y";
export { z } from "./z";
const m = require("./m");
await import("./dynamic");
```

Dynamic imports with statically resolvable string values can be tracked.

Unknown dynamic expressions should lower confidence or trigger configured fallback behavior.

---

# 15. Python Parsing

Recognize:

```python
import module
from package.submodule import symbol
```

Resolve modules according to repository search roots.

---

# 16. Dependency Graph

Represent:

```text
G = (V, E)
```

where:

```text
V = repository file nodes
E = directed dependency relationships
```

If:

```text
A imports B
```

store:

```text
A → B
```

Also maintain:

```text
reverse[B].insert(A)
```

because impact propagation proceeds through dependents.

---

# 17. Graph Storage

In-memory graph:

```cpp
using FileId = std::uint64_t;

std::unordered_map<FileId, std::vector<FileId>> dependencies;
std::unordered_map<FileId, std::vector<FileId>> reverseDependencies;
```

For large repositories, IDs should be compact integers rather than repeated path strings.

Persist edges in PostgreSQL.

---

# 18. Incremental Graph Rebuild

Do not rebuild every file for every PR.

On each run:

1. identify changed files,
2. reparse only changed or newly added files,
3. remove stale edges originating from modified files,
4. create new edges,
5. remove deleted nodes,
6. update reverse edges,
7. recalculate dependent fingerprints where necessary.

Full graph rebuild remains available as a repair operation.

---

# 19. Cycle Handling

Source graphs may contain cycles.

Example:

```text
A → B
B → C
C → A
```

BFS/DFS impact traversal must use a visited set.

The graph analysis module must also implement:

```text
Tarjan Strongly Connected Components
```

SCC output may be used for:

- diagnostics,
- graph visualization,
- dependency hotspot analysis,
- component condensation.

---

# 20. Test Discovery

Configuration:

```yaml
tests:
  patterns:
    - "**/*_test.cpp"
    - "**/*.test.ts"
    - "**/*.spec.ts"
    - "**/test_*.py"
```

Framework adapters may discover individual cases when supported.

Examples:

```text
GoogleTest
Catch2
Jest
Vitest
Pytest
```

---

# 21. Test Framework Adapter Interface

```cpp
class ITestFrameworkAdapter {
public:
    virtual ~ITestFrameworkAdapter() = default;

    virtual std::vector<TestCase> discover(
        const RepositoryContext&
    ) = 0;

    virtual TestCommand createCommand(
        const std::vector<TestCase>& tests
    ) = 0;

    virtual TestExecutionResult parseResult(
        const ProcessResult&
    ) = 0;
};
```

---

# 22. Impact Analysis

Input:

```text
changed file set C
```

Output:

```text
affected node set A
```

Algorithm:

```text
queue = changed files
affected = changed files

while queue not empty:
    current = queue.pop()

    for each dependent in reverseGraph[current]:
        if dependent not visited:
            visited.insert(dependent)
            affected.insert(dependent)
            queue.push(dependent)
```

Worst-case complexity:

```text
O(V + E)
```

---

# 23. Explainability Paths

For every affected test, store at least one predecessor chain.

Example:

```text
checkout_test.cpp
→ checkout.cpp
→ payment_service.cpp
→ CHANGED
```

During BFS:

```text
parent[dependent] = current
```

Then reconstruct the path from selected test to changed source.

---

# 24. Impact Confidence

Every selection analysis has a confidence result:

```text
HIGH
MEDIUM
LOW
UNSAFE
```

Example causes of degraded confidence:

```text
unresolved import
dynamic runtime plugin loading
generated file dependency
unknown build macro
parser failure
unsupported language
```

`UNSAFE` must trigger full-suite fallback unless explicitly overridden.

---

# 25. Safety Fallback Rules

Example configuration:

```yaml
fallback:
  fullSuiteWhenChanged:
    - "package.json"
    - "package-lock.json"
    - "pnpm-lock.yaml"
    - "CMakeLists.txt"
    - "cmake/**"
    - "requirements.txt"
    - "pyproject.toml"
    - ".github/workflows/**"
    - "testgraph.yml"
```

Fallback triggers also include:

- graph corruption,
- parser crash,
- unresolved repository root,
- unsupported generated dependency,
- changed dependency lock file,
- incompatible environment fingerprint,
- explicit user request.

---

# 26. Always-Run Tests

Configuration:

```yaml
tests:
  alwaysRun:
    - "tests/security/**"
    - "tests/smoke/**"
```

These tests execute regardless of impact analysis.

---

# 27. Execution Plan

The plan contains jobs, dependencies, estimates, cache status, and priority.

Example:

```json
{
  "jobs": [
    {
      "id": "build",
      "type": "BUILD",
      "estimatedDurationMs": 18000,
      "dependsOn": []
    },
    {
      "id": "payment-tests",
      "type": "TEST",
      "estimatedDurationMs": 32000,
      "dependsOn": ["build"]
    }
  ]
}
```

---

# 28. Job Types

```text
CHECKOUT
SETUP
BUILD
STATIC_ANALYSIS
UNIT_TEST
INTEGRATION_TEST
E2E_TEST
POST_PROCESS
REPORT
```

---

# 29. DAG Construction

Execution graph:

```text
Build
 ├─ Unit Tests
 ├─ Static Analysis
 └─ Integration Setup
      └─ Integration Tests
           └─ E2E Tests
```

Each node stores:

```cpp
struct JobNode {
    JobId id;
    JobType type;
    std::vector<JobId> dependencies;
    std::vector<JobId> dependents;
    std::chrono::milliseconds estimatedDuration;
    int priority;
};
```

---

# 30. DAG Scheduling

Use indegree counting.

```text
remainingDependencies[job] = dependencies.size()
```

When zero:

```text
job → ready queue
```

When a job finishes successfully:

```text
for dependent:
    remainingDependencies[dependent]--
    if 0:
        ready.push(dependent)
```

A failed prerequisite may:

- block dependent job,
- mark it skipped,
- or continue according to policy.

---

# 31. Parallel Scheduler

The scheduler must support multiple policies.

## 31.1 Default Policy

Use:

```text
Longest Processing Time First
```

for ready independent tests.

Sort by historical estimated duration descending.

Assign the next test to the worker with the smallest predicted workload.

---

# 32. Worker Pool

Local mode:

```text
C++ scheduler
     ↓
N worker threads/process executors
```

Repository tests should usually run as child processes rather than in scheduler threads.

Workers manage:

- process start,
- stdout/stderr capture,
- timeout,
- exit status,
- cancellation,
- result parsing.

---

# 33. Process Execution

Use a robust process abstraction.

Required features:

```text
working directory
environment variables
stdin control
stdout capture
stderr capture
timeout
signal forwarding
graceful cancellation
forced termination
exit code
resource usage
```

Do not execute test commands using unsafe shell concatenation.

---

# 34. Remote Worker Architecture

Complete version must support remote workers.

```text
Scheduler
   │
   ├── Worker Agent 1
   ├── Worker Agent 2
   └── Worker Agent 3
```

Worker registration:

```text
POST /internal/workers/register
```

Heartbeat:

```text
POST /internal/workers/{id}/heartbeat
```

Job polling or dispatch:

```text
POST /internal/jobs/claim
```

Completion:

```text
POST /internal/jobs/{id}/complete
```

---

# 35. Worker States

```text
REGISTERING
IDLE
BUSY
DRAINING
DEAD
```

---

# 36. Worker Heartbeats

Each worker sends a heartbeat periodically.

Scheduler stores:

```text
lastHeartbeatAt
currentJobId
availableCpu
availableMemory
```

If heartbeat timeout expires:

```text
worker → DEAD
```

Running jobs are requeued if safe.

---

# 37. Idempotent Job Completion

Every job completion request must include:

```text
job id
attempt number
execution token
```

Duplicate completion messages must not duplicate database results.

Use unique constraints or transactional checks.

---

# 38. Test Runtime History

Persist:

```text
test
commit
run
worker
attempt
duration
status
timestamp
cache status
failure signature
```

Maintain rolling statistics:

```text
mean
median
P50
P90
P95
recent failure count
recent retry success rate
```

---

# 39. Runtime Estimation

Initial estimate:

```text
moving average of recent successful executions
```

Future enhancement:

```text
EWMA
```

Example:

```text
estimate =
alpha * latest_duration
+
(1 - alpha) * previous_estimate
```

---

# 40. Critical Path Scheduling

For DAG jobs, calculate estimated critical-path weight.

```text
criticalWeight(node) =
duration(node)
+
max(criticalWeight(child))
```

Use this as a scheduling priority for dependencies where it improves makespan.

---

# 41. Test Cache

The cache must only reuse trusted **passing** results by default.

Cache key inputs:

```text
test source
transitive dependency fingerprints
test command
compiler/runtime version
dependency lockfile
configuration fingerprint
environment fingerprint
tool version
```

---

# 42. Dependency Fingerprints

For file:

```text
fingerprint(file) =
HASH(
  content_hash(file)
  +
  sorted(fingerprint(dep_1), ..., fingerprint(dep_n))
)
```

Cycles must be handled via SCC condensation or cycle-stable hashing.

---

# 43. SCC-Aware Fingerprinting

For strongly connected component:

```text
SCC_HASH =
HASH(
    sorted(content_hash(all nodes))
    +
    sorted(external dependency hashes)
)
```

Each member can reference the SCC hash.

This avoids recursive hashing loops.

---

# 44. Cache Key

Conceptually:

```text
test_cache_key =
SHA256(
    test_identity
    + dependency_fingerprint
    + test_runner_version
    + runtime_version
    + build_configuration
    + environment_fingerprint
)
```

---

# 45. Cache Backends

Use:

```text
Redis → fast metadata
PostgreSQL → durable metadata/history
Object/file storage → optional large artifacts
```

The initial complete deployment may store result metadata in PostgreSQL and use Redis for fast lookup.

---

# 46. Cache Invalidation

Invalidation occurs when:

- test changes,
- transitive dependency changes,
- runtime version changes,
- compiler changes,
- lockfile changes,
- test command changes,
- relevant environment changes,
- ChangeLens cache schema version changes.

---

# 47. Cache Metrics

Track:

```text
cache lookups
cache hits
cache misses
cache bypasses
cache invalidations
cache hit rate
tests avoided
worker seconds avoided
```

---

# 48. Flaky Test Detection

A probable flaky test is one that exhibits inconsistent outcome with effectively identical relevant inputs.

Example:

```text
Attempt 1 → FAIL
Attempt 2 → PASS
```

Store this event.

---

# 49. Retry Policy

Example:

```yaml
flaky:
  enabled: true
  maxRetries: 2
  retryOnlyKnownFlaky: false
```

Flow:

```text
test fails
   ↓
check retry policy
   ↓
retry
   ↓
pass?
 /    \
yes    no
 |      |
flaky   confirmed failure
```

Original failure remains visible.

---

# 50. Flake Score

Initial score:

```text
retry_successes_after_failure /
initial_failures
```

Weighted future score may include:

- recency,
- failure frequency,
- platform,
- worker,
- commit similarity,
- failure signature stability.

---

# 51. Test Quarantine

Optional policy:

```yaml
flaky:
  quarantine:
    enabled: true
    threshold: 0.85
```

Quarantined failures must still appear as warnings.

Do not silently ignore them.

---

# 52. Failure Signature Engine

Normalize failure text:

- remove timestamps,
- normalize temp paths,
- normalize line-number noise where appropriate,
- preserve semantic error type.

Compute:

```text
signature = HASH(test + normalized error)
```

Group repeated failures by signature.

---

# 53. GitHub App Integration

ChangeLens should be installed as a GitHub App.

Required permissions:

- repository metadata read,
- contents read,
- pull requests read,
- checks read/write.

Webhook events:

```text
pull_request
push
installation
installation_repositories
```

---

# 54. Webhook Security

Every webhook must verify GitHub signature.

Reject:

```text
missing signature
invalid signature
oversized payload
unknown event type
```

Use constant-time comparison for signature verification.

---

# 55. Pull Request Flow

```text
PR opened / updated
      ↓
verify webhook
      ↓
create run
      ↓
cancel stale run for same PR
      ↓
fetch base/head
      ↓
detect changes
      ↓
update dependency graph
      ↓
impact analysis
      ↓
cache resolution
      ↓
execution DAG
      ↓
schedule
      ↓
execute
      ↓
retry flaky failures
      ↓
persist
      ↓
publish GitHub Check
```

---

# 56. GitHub Check Output

Example:

```text
ChangeLens

Status: Passed

Changed files:             8
Affected files:           43
Repository tests:        426
Affected tests:           71
Cache hits:               29
Executed tests:           42

Passed:                   41
Probable flaky:            1
Failed:                    0

Estimated full-suite:  18m 42s
Actual wall time:       4m 18s
Estimated time saved:  14m 24s
```

---

# 57. Test Selection Explanation

For each selected test:

```text
checkout_test.cpp

Selected because:

checkout_test.cpp
→ checkout.cpp
→ payment_service.cpp
→ changed in PR
```

This explanation must be available in:

- API,
- CLI,
- dashboard,
- GitHub report link.

---

# 58. Run Cancellation

If a new commit is pushed to the same PR:

```text
Run A → stale
Run B → current
```

Run A must be cancelled.

Cancellation propagates to:

- scheduler,
- workers,
- test processes,
- pending retries.

---

# 59. Persistence Model

PostgreSQL tables:

```text
repositories
commits
pull_requests
runs
file_nodes
dependency_edges
tests
test_statistics
test_executions
cache_entries
jobs
job_dependencies
workers
failure_signatures
audit_runs
```

---

# 60. Database Schema — Repository

```sql
repositories
------------
id
github_repository_id
owner
name
default_branch
installation_id
created_at
updated_at
```

---

# 61. File Nodes

```sql
file_nodes
----------
id
repository_id
path
kind
language
content_hash
dependency_fingerprint
created_at
updated_at
```

Unique:

```text
(repository_id, path)
```

---

# 62. Dependency Edges

```sql
dependency_edges
----------------
id
repository_id
source_file_id
target_file_id
dependency_type
created_at
```

Unique:

```text
(repository_id, source_file_id, target_file_id, dependency_type)
```

---

# 63. Runs

```sql
runs
----
id
repository_id
pull_request_number
base_sha
head_sha
status
analysis_confidence
full_suite_fallback
started_at
completed_at
cancelled_at
```

---

# 64. Jobs

```sql
jobs
----
id
run_id
job_type
status
estimated_duration_ms
actual_duration_ms
attempt
worker_id
started_at
completed_at
```

---

# 65. Tests

```sql
tests
-----
id
repository_id
file_node_id
framework
suite_name
test_name
created_at
updated_at
```

---

# 66. Test Executions

```sql
test_executions
---------------
id
run_id
test_id
job_id
worker_id
status
duration_ms
attempt
cache_hit
failure_signature_id
created_at
```

---

# 67. Test Statistics

```sql
test_statistics
---------------
test_id
execution_count
pass_count
failure_count
retry_success_count
average_duration_ms
p50_duration_ms
p95_duration_ms
flake_score
last_execution_at
updated_at
```

---

# 68. Redis Usage

Redis should handle ephemeral state only.

Examples:

```text
ready job queue
worker heartbeat keys
distributed locks
run cancellation flags
short-lived cache lookup
rate limiting
```

PostgreSQL remains source of durable truth.

---

# 69. REST API

Base:

```text
/api/v1
```

Repositories:

```text
GET  /repositories
GET  /repositories/{id}
POST /repositories/{id}/analyze
GET  /repositories/{id}/graph
GET  /repositories/{id}/tests
```

Runs:

```text
GET  /runs
GET  /runs/{id}
POST /runs/{id}/cancel
GET  /runs/{id}/affected
GET  /runs/{id}/execution-plan
GET  /runs/{id}/jobs
GET  /runs/{id}/metrics
```

Tests:

```text
GET /tests/{id}
GET /tests/{id}/history
GET /tests/{id}/failures
```

Flaky:

```text
GET /flaky-tests
```

Graph:

```text
GET /repositories/{id}/graph/subgraph
```

GitHub:

```text
POST /webhooks/github
```

Workers:

```text
POST /internal/workers/register
POST /internal/workers/{id}/heartbeat
POST /internal/jobs/claim
POST /internal/jobs/{id}/complete
```

---

# 70. API Error Format

```json
{
  "error": {
    "code": "GRAPH_ANALYSIS_FAILED",
    "message": "Unable to resolve dependency graph",
    "requestId": "req_..."
  }
}
```

---

# 71. CLI

Binary:

```bash
testgraph
```

Commands:

```bash
testgraph init
testgraph analyze
testgraph graph
testgraph affected <base> <head>
testgraph explain <test>
testgraph run
testgraph benchmark
testgraph doctor
testgraph cache stats
testgraph cache clear
```

---

# 72. `testgraph analyze`

Example:

```text
Repository analysis complete

Files scanned:           12,481
Source files:             8,720
Tests:                    2,106
Dependency edges:        58,492
SCC groups:                  19
Unresolved imports:          11
Analysis confidence:       HIGH
Duration:                  3.8s
```

---

# 73. `testgraph affected`

```bash
testgraph affected main HEAD
```

Output:

```text
Changed files: 4
Affected files: 83
Affected tests: 27
Always-run tests: 3
Full-suite fallback: no
```

---

# 74. `testgraph explain`

```bash
testgraph explain tests/payment/refund_test.cpp
```

Output:

```text
Selected because:

tests/payment/refund_test.cpp
→ src/payment/refund.cpp
→ src/payment/payment_service.cpp
→ changed in current diff
```

---

# 75. Configuration File

`testgraph.yml`

```yaml
version: 1

project:
  name: commerce-service

languages:
  - cpp
  - typescript

tests:
  frameworks:
    - googletest
    - vitest

  patterns:
    - "**/*_test.cpp"
    - "**/*.test.ts"

  alwaysRun:
    - "tests/security/**"

execution:
  workers: 8
  timeoutSeconds: 900

  maxConcurrentRunsPerRepository: 2

cache:
  enabled: true
  ttlSeconds: 604800

flaky:
  enabled: true
  maxRetries: 2
  quarantine:
    enabled: false

fallback:
  fullSuiteWhenChanged:
    - "CMakeLists.txt"
    - "cmake/**"
    - "package-lock.json"
    - "pnpm-lock.yaml"
    - ".github/workflows/**"
    - "testgraph.yml"

validation:
  auditRate: 0.05

ignore:
  - "build/**"
  - "dist/**"
  - "node_modules/**"
  - ".git/**"
```

---

# 76. Dashboard

Frontend:

```text
React
TypeScript
Tailwind CSS
Recharts
React Flow
```

Main pages:

```text
Overview
Repositories
Runs
Run Details
Tests
Flaky Tests
Failures
Dependency Explorer
Workers
Settings
```

---

# 77. Dashboard Overview

Cards:

```text
CI Runs
Tests Avoided
CI Time Saved
Cache Hit Rate
Median PR Runtime
Flaky Tests
Selection Accuracy
Worker Utilization
```

Charts:

```text
CI duration trend
Tests executed vs total
Cache hit trend
Failure trend
Flaky-test trend
```

---

# 78. Run Details

Display:

```text
PR number
head SHA
base SHA
status
changed files
affected files
selected tests
cache hits
executed tests
worker count
estimated full runtime
actual runtime
saved runtime
```

Tabs:

```text
Summary
Affected Graph
Execution DAG
Tests
Failures
Workers
Logs
```

---

# 79. Dependency Graph Explorer

Node categories:

```text
Changed
Affected Source
Selected Test
Unaffected
Config
Build
```

Features:

- zoom,
- pan,
- search,
- filter,
- click node,
- show dependencies,
- show dependents,
- shortest path to changed file,
- SCC grouping.

---

# 80. Worker Timeline

Gantt-style timeline:

```text
Worker 1 | build | payment | refund | integration |
Worker 2 |      | auth    | orders |             |
Worker 3 |      | cart    | e2e                    |
```

Display idle gaps and predicted vs actual duration.

---

# 81. Flaky Test Dashboard

Columns:

```text
Test
Framework
Recent Runs
Failures
Retry Passes
Flake Score
Average Runtime
Last Failure
```

Details page shows:

```text
execution history
failure signatures
retry outcomes
commits
workers
```

---

# 82. Slowest Tests Dashboard

Columns:

```text
Test
Average
P50
P95
Run Count
Failure Rate
Last Runtime
```

---

# 83. Correctness Audit Mode

Selective testing has a serious risk: missing a failing test.

To measure this, periodically run a complete suite after the optimized run.

Configuration:

```yaml
validation:
  auditRate: 0.05
```

For selected runs:

```text
optimized test selection
        ↓
optimized result
        ↓
full test suite
        ↓
compare results
```

---

# 84. Selection Miss

A selection miss occurs when:

```text
ChangeLens skipped test X
```

but:

```text
full suite shows X fails because of current change
```

Store:

```text
changed files
missed test
dependency graph state
parser confidence
failure signature
commit
```

---

# 85. Selection Accuracy Metric

```text
accuracy =
1 - selection_misses / audited_runs
```

Also track:

```text
missed_tests_total
audit_runs_total
```

No performance claim is valid without correctness evidence.

---

# 86. Runtime Coverage Integration

Static dependency analysis is the first mechanism.

Complete system should optionally support dynamic coverage mapping:

```text
test → files executed during test
```

Then affected-test candidates become:

```text
static dependency tests
UNION
runtime coverage tests
UNION
always-run safety tests
```

This hybrid approach reduces false negatives.

---

# 87. Coverage Map

Persist:

```text
test_id
file_id
last_observed_at
observation_count
```

A test that historically executes a changed file should be included even if static analysis misses the relationship.

---

# 88. Risk-Based Expansion

Optional complete feature:

```text
risk score =
change size
× dependency centrality
× historical failure rate
× uncertainty
```

High-risk changes may cause selection expansion.

Example:

```text
database abstraction changed
```

The system may include additional integration or smoke tests.

---

# 89. Graph Centrality

Calculate:

```text
in-degree
out-degree
transitive dependent count
```

Optional:

```text
PageRank-style importance
```

Expose dependency hotspots.

---

# 90. Dependency Hotspots

Dashboard:

```text
src/common/config.cpp       513 dependents
src/db/client.cpp           441 dependents
src/auth/session.cpp        276 dependents
```

Useful for:

- architecture review,
- refactoring decisions,
- CI impact prediction.

---

# 91. Execution Isolation

Repository code must not run directly in the server process.

Production worker executes jobs inside Docker.

Container policy:

```text
CPU limit
memory limit
wall-clock timeout
temporary filesystem
restricted mounts
limited privileges
network policy
```

---

# 92. Docker Worker Flow

```text
worker claims job
      ↓
prepare run directory
      ↓
start container
      ↓
mount repository read/write as configured
      ↓
execute test command
      ↓
capture result
      ↓
destroy container
      ↓
upload logs
      ↓
complete job
```

---

# 93. Security

Required:

- webhook signature verification,
- secret redaction,
- no shell-string concatenation,
- command allowlisting/config validation,
- repository path validation,
- container isolation,
- least-privilege GitHub App permissions,
- database parameterized queries,
- internal worker authentication,
- API rate limiting,
- request-size limits.

---

# 94. Secrets

Environment variables:

```text
TESTGRAPH_GITHUB_APP_ID
TESTGRAPH_GITHUB_PRIVATE_KEY
TESTGRAPH_GITHUB_WEBHOOK_SECRET
TESTGRAPH_DATABASE_URL
TESTGRAPH_REDIS_URL
TESTGRAPH_INTERNAL_WORKER_TOKEN
```

Secrets must never be:

- committed,
- printed in logs,
- returned by APIs,
- embedded into dashboard bundles.

---

# 95. Observability

Logs must be structured.

Example:

```json
{
  "level": "info",
  "event": "test_completed",
  "requestId": "req_123",
  "runId": "run_456",
  "repositoryId": "repo_8",
  "workerId": "worker_2",
  "test": "payment_test.cpp",
  "durationMs": 6124,
  "status": "PASS"
}
```

---

# 96. Metrics

Prometheus-compatible metrics:

```text
testgraph_runs_total
testgraph_runs_failed_total
testgraph_graph_analysis_seconds
testgraph_impact_analysis_seconds
testgraph_tests_selected_total
testgraph_tests_executed_total
testgraph_tests_cache_hit_total
testgraph_tests_skipped_total
testgraph_worker_busy_seconds
testgraph_worker_idle_seconds
testgraph_job_retry_total
testgraph_flaky_tests_total
testgraph_selection_miss_total
```

---

# 97. Health Endpoints

```text
GET /health/live
GET /health/ready
```

Readiness checks:

```text
PostgreSQL
Redis
required configuration
```

---

# 98. Error Categories

```text
USER_CONFIGURATION_ERROR
REPOSITORY_ERROR
PARSER_ERROR
GRAPH_ERROR
CACHE_ERROR
TEST_FAILURE
INFRA_FAILURE
WORKER_FAILURE
GITHUB_API_ERROR
DATABASE_ERROR
INTERNAL_ERROR
```

---

# 99. Infrastructure Failure Behavior

Examples:

```text
worker crash
container runtime unavailable
repository checkout fails
database unavailable
GitHub API timeout
```

Infra failures must not be mislabeled as test failures.

---

# 100. Retry Strategy

Retryable infrastructure operations should use bounded exponential backoff.

Do not retry:

- deterministic config errors,
- invalid repository paths,
- authentication failures without credential refresh path.

---

# 101. Complete Testing Strategy

ChangeLens itself must have strong automated testing.

## Unit Tests

Cover:

- graph construction,
- reverse graph,
- BFS,
- DFS,
- SCC detection,
- path explanation,
- dependency fingerprinting,
- SCC hashing,
- cache key creation,
- LPT scheduling,
- critical path calculation,
- DAG indegree transitions,
- worker state transitions,
- cancellation,
- flake score,
- failure signature normalization,
- config parsing,
- Git diff mapping.

---

# 102. Integration Tests

Cover:

```text
repository → parser → graph
git diff → impact analysis
impact analysis → selected tests
selected tests → cache resolver
cache resolver → execution plan
execution plan → scheduler
scheduler → worker
worker → result persistence
result → flaky processor
webhook → complete run
```

---

# 103. End-to-End Tests

Use fixture repositories.

Scenario:

1. create base commit,
2. create branch,
3. modify payment module,
4. calculate diff,
5. verify affected tests,
6. run selected tests,
7. verify cache on second execution,
8. simulate flaky test,
9. verify retry,
10. verify final report.

---

# 104. Failure Recovery Tests

Simulate:

```text
worker dies mid-test
database becomes unavailable
Redis temporarily unavailable
GitHub reporting fails
new commit cancels current run
duplicate job completion
duplicate webhook delivery
```

---

# 105. GitHub Webhook Idempotency

GitHub may redeliver events.

Store delivery IDs.

If delivery already processed:

```text
return success without creating duplicate run
```

---

# 106. Benchmarks

Provide a synthetic repository generator.

```bash
testgraph benchmark generate \
  --files 10000 \
  --tests 3000 \
  --edges 50000
```

Benchmark:

```text
full graph build
incremental graph update
impact traversal
SCC detection
fingerprint recomputation
scheduler planning
cache lookup throughput
```

---

# 107. Performance Targets

Target examples:

```text
10,000 files graph build       < 5 s
Incremental update, 10 files   < 1 s
Impact traversal               < 500 ms
1,000-test scheduling plan     < 100 ms
Webhook acknowledgement        < 500 ms
```

These must be measured, not fabricated.

---

# 108. Scale Targets

Normal:

```text
50,000 files
10,000 tests
250,000 edges
16 local workers
```

Stretch:

```text
100,000+ files
30,000+ tests
500,000+ edges
distributed workers
```

---

# 109. CI Performance Metrics

## Selection Ratio

```text
selected_tests / total_tests
```

## Execution Ratio

```text
executed_tests / total_tests
```

## Cache Hit Rate

```text
cache_hits / cache_eligible_tests
```

## Wall-Clock Reduction

```text
1 - optimized_runtime / full_suite_estimate
```

## Worker Utilization

```text
worker_busy_time / worker_available_time
```

## Correctness Miss Rate

```text
selection_misses / audited_runs
```

---

# 110. Full-Suite Baseline

Store baseline runtime for repository.

Update using actual full-suite runs.

Do not calculate savings from unrealistic constants.

---

# 111. Deployment Architecture

Local development:

```text
Docker Compose
```

Services:

```text
testgraph-server
testgraph-worker
postgres
redis
dashboard
```

Production:

```text
reverse proxy
server replicas
worker pool
PostgreSQL
Redis
dashboard/static frontend
```

---

# 112. Docker Compose Development Stack

Example components:

```text
postgres:5432
redis:6379
server:8080
dashboard:3000
worker-1
worker-2
```

---

# 113. Database Migrations

Migration files:

```text
migrations/
  001_initial.sql
  002_test_statistics.sql
  003_cache.sql
  ...
```

Migrations must run transactionally where supported.

---

# 114. Authentication

Dashboard user authentication can initially use GitHub OAuth.

Authorization must ensure users only access repositories they are permitted to view.

---

# 115. Repository Installation Flow

```text
Sign in with GitHub
      ↓
Install GitHub App
      ↓
Select repositories
      ↓
ChangeLens detects languages
      ↓
Detect test frameworks
      ↓
Generate suggested config
      ↓
Run baseline analysis
      ↓
Build initial graph
      ↓
Run full suite baseline
      ↓
Enable optimized PR execution
```

---

# 116. Test Framework Detection

Examples:

C++:

```text
CMake + GoogleTest
Catch2
CTest
```

JS/TS:

```text
Jest
Vitest
```

Python:

```text
Pytest
```

The system should recommend configuration rather than silently guessing when ambiguous.

---

# 117. C++ / CTest Adapter

Complete C++ support should include:

```text
CTest discovery
GoogleTest integration
test executable mapping
test name execution
```

For GoogleTest:

```bash
./unit_tests --gtest_filter=Payment.*
```

For CTest:

```bash
ctest -R Payment
```

---

# 118. Build Dependency Layer

For C++ repositories, file-level includes alone may not capture build target relationships.

Parse CMake metadata where possible.

Graph can include:

```text
source file
header
library target
executable target
test target
```

Example:

```text
payment.cpp
   ↓
payment_lib
   ↓
payment_tests
```

This improves correctness.

---

# 119. Build Graph Model

Introduce node types:

```text
FILE
BUILD_TARGET
TEST
CONFIG
```

Edges:

```text
FILE_IMPORTS_FILE
TARGET_CONTAINS_FILE
TARGET_LINKS_TARGET
TEST_EXECUTES_TARGET
```

This makes C++ support significantly more realistic.

---

# 120. CMake Integration

Support a helper that obtains CMake File API information.

Use CMake File API to inspect:

```text
targets
sources
dependencies
compile groups
```

This is preferred over fragile parsing of arbitrary CMake syntax.

---

# 121. Hybrid C++ Impact Analysis

For C++:

```text
changed .cpp/.hpp
     ↓
include graph
     +
CMake target graph
     ↓
affected targets
     ↓
affected test executables
     ↓
individual tests when available
```

---

# 122. Monorepo Support

Repository may contain:

```text
services/
libraries/
apps/
packages/
```

Maintain graph across modules.

Allow configured roots:

```yaml
project:
  roots:
    - services
    - libraries
```

---

# 123. Multiple Language Support

Parser registry:

```cpp
ParserRegistry registry;

registry.add(std::make_unique<CppParser>());
registry.add(std::make_unique<TreeSitterTypeScriptParser>());
registry.add(std::make_unique<TreeSitterPythonParser>());
```

The graph format is language-agnostic.

---

# 124. Generated Files

Generated sources must be handled explicitly.

Configuration:

```yaml
generated:
  patterns:
    - "generated/**"
```

If generated source depends on an upstream schema/config, model that dependency if known.

Otherwise degrade confidence.

---

# 125. Dynamic Dependencies

Examples:

```text
reflection
plugin discovery
runtime module loading
filesystem scanning
dependency injection by string
```

These cannot always be resolved statically.

Mitigations:

- runtime coverage,
- always-run tests,
- risk expansion,
- full-suite fallback,
- audit mode.

---

# 126. AI-Assisted Failure Analysis — Optional

AI is optional and strictly secondary.

Input:

```text
changed files
failed test
stack trace
dependency path
historical matching signatures
```

Output:

```text
possible cause summary
related changed file
suggested investigation path
```

AI must never decide whether a required test can be skipped.

---

# 127. Internal Concurrency Design

Suggested scheduler components:

```text
RunCoordinator
JobGraph
ReadyQueue
WorkerRegistry
AssignmentEngine
ResultCollector
CancellationToken
```

Use explicit ownership and avoid shared mutable global state.

---

# 128. Thread Safety

Shared scheduler state protected by:

```text
mutex
condition_variable
atomics
```

Do not hold locks while:

- waiting on network,
- running tests,
- writing large logs,
- performing database transactions.

---

# 129. Event Loop Option

HTTP may use framework-managed async event loops.

Domain services should not rely on specific HTTP implementation details.

---

# 130. Graceful Shutdown

On SIGTERM:

1. stop accepting new webhooks,
2. stop assigning new jobs,
3. mark workers draining,
4. allow active jobs to complete for configured grace period,
5. persist state,
6. shut down.

---

# 131. Logging Retention

Store detailed worker logs for a bounded retention period.

Large raw logs should eventually live outside PostgreSQL.

Database stores metadata and references.

---

# 132. Dashboard Filters

Support filtering by:

```text
repository
branch
date
status
test framework
worker
cache hit
flaky
failure signature
```

---

# 133. Search

Search by:

```text
file path
test name
commit SHA
PR number
failure signature
```

---

# 134. Settings

Repository settings:

```text
worker count
test framework
test patterns
always-run patterns
cache enabled
cache TTL
retry count
audit rate
fallback patterns
timeouts
```

---

# 135. Admin Operations

CLI/admin API:

```text
rebuild graph
invalidate repository cache
recompute test statistics
mark test quarantined
remove stale worker
rerun failed job
```

---

# 136. Data Retention

Configurable:

```text
run history retention
test log retention
cache TTL
worker heartbeat retention
```

Historical aggregate statistics can outlive raw logs.

---

# 137. Complete Implementation Phases

This is not an MVP cutoff. These phases build the full product sequentially.

## Phase 1 — Core C++ Foundation

Build:

- CMake project,
- logging,
- configuration,
- database layer,
- Redis layer,
- CLI foundation,
- HTTP service.

## Phase 2 — Git Repository Engine

Build:

- clone,
- fetch,
- worktree,
- diff,
- commit metadata,
- cancellation-safe cleanup.

## Phase 3 — C++ Static Analysis

Build:

- include graph,
- Clang integration,
- CMake File API integration,
- target graph,
- test target mapping.

## Phase 4 — Generic Parser Layer

Add:

- Tree-sitter infrastructure,
- TypeScript parser,
- Python parser.

## Phase 5 — Graph Engine

Build:

- compact graph IDs,
- reverse adjacency,
- incremental updates,
- SCC,
- path reconstruction,
- persistence.

## Phase 6 — Test Discovery

Build:

- CTest,
- GoogleTest,
- Catch2,
- Jest/Vitest,
- Pytest adapters.

## Phase 7 — Impact Engine

Build:

- changed-file analysis,
- transitive dependents,
- always-run rules,
- fallback rules,
- explainability,
- confidence.

## Phase 8 — Local Execution Engine

Build:

- child-process abstraction,
- timeouts,
- log capture,
- cancellation,
- result adapters.

## Phase 9 — DAG Scheduler

Build:

- job graph,
- indegree tracking,
- ready queue,
- LPT,
- critical path,
- worker assignment.

## Phase 10 — Persistence & Statistics

Build:

- run history,
- test executions,
- runtime aggregates,
- failure signatures.

## Phase 11 — Dependency-Aware Cache

Build:

- content hashes,
- SCC-aware fingerprints,
- cache keys,
- lookup,
- invalidation,
- metrics.

## Phase 12 — Flaky Test Intelligence

Build:

- retries,
- flake score,
- quarantine option,
- dashboard support.

## Phase 13 — GitHub App

Build:

- app auth,
- webhook verification,
- PR events,
- checks,
- stale-run cancellation,
- idempotency.

## Phase 14 — Dashboard

Build all pages:

- overview,
- repositories,
- runs,
- run details,
- graph,
- tests,
- flaky tests,
- workers,
- settings.

## Phase 15 — Docker Worker Runtime

Build:

- worker agent,
- container lifecycle,
- resource limits,
- secure execution,
- heartbeat.

## Phase 16 — Distributed Scheduling

Build:

- Redis-backed ready queue,
- job claims,
- worker recovery,
- duplicate-completion protection,
- multi-run concurrency.

## Phase 17 — Correctness Audit Mode

Build:

- sampled full-suite validation,
- selection miss detection,
- accuracy metrics.

## Phase 18 — Runtime Coverage

Build:

- test-to-file coverage map,
- hybrid static + dynamic selection.

## Phase 19 — Performance Engineering

Build:

- benchmark generator,
- profiling,
- graph memory optimization,
- database indexes,
- scheduler benchmarks.

## Phase 20 — Production Hardening

Build:

- security review,
- rate limiting,
- retention policies,
- health checks,
- graceful shutdown,
- backup/restore documentation,
- operational runbooks.

---

# 138. Definition of Complete Project

The project is considered complete only when the following entire flow works:

1. user installs GitHub App,
2. repository is registered,
3. language and tests are discovered,
4. initial dependency graph is generated,
5. baseline full suite is executed,
6. pull request is opened,
7. webhook is verified,
8. changed files are identified,
9. graph updates incrementally,
10. affected files are computed,
11. selected tests are generated,
12. always-run policies are applied,
13. unsafe conditions trigger full-suite fallback,
14. cache eligibility is computed,
15. execution DAG is generated,
16. tests are scheduled using historical duration,
17. jobs are executed in Docker workers,
18. worker failures are recoverable,
19. test retries are performed according to policy,
20. flaky tests are classified,
21. results are persisted,
22. metrics are emitted,
23. GitHub Check is updated,
24. dashboard shows run details,
25. dependency path explains each selected test,
26. worker timeline is visible,
27. CI saved time is calculated,
28. cache hit rate is visible,
29. flaky-test history is visible,
30. audit mode occasionally runs full suite,
31. selection misses are tracked,
32. runtime coverage can augment static analysis.

---

# 139. Acceptance Criteria

## Dependency Graph

- correctly stores direct dependencies,
- reverse traversal works,
- cycles do not cause infinite traversal,
- SCCs are detected,
- incremental changes update affected edges.

## C++ Support

- C++ includes resolve correctly for project files,
- CMake target dependencies are represented,
- changed headers affect dependent translation units,
- affected test executables are identified.

## Impact Analysis

- expected fixture tests are selected,
- unrelated tests are excluded,
- explainability path exists for each selection,
- unsafe conditions trigger fallback.

## Scheduler

- DAG dependencies are respected,
- independent jobs execute concurrently,
- predicted long tests are distributed across workers,
- stale runs can cancel active processes.

## Workers

- worker heartbeat works,
- dead worker is detected,
- recoverable jobs are requeued,
- duplicate completion is idempotent.

## Cache

- unchanged dependency graph permits a hit,
- transitive dependency changes invalidate the hit,
- runtime/config change invalidates the hit,
- failing results are not reused as passing cache entries.

## GitHub

- webhook signatures are verified,
- duplicate webhook deliveries do not duplicate runs,
- PR checks update from queued to final status,
- new PR commit cancels stale run.

## Flaky Tests

- fail→pass retry is recorded,
- original failure remains visible,
- flake score updates.

## Audit

- sampled runs execute full suite,
- skipped failing tests are classified as selection misses,
- accuracy metrics update.

## Dashboard

- all persisted runs are visible,
- graph visualization loads,
- worker timeline loads,
- cache metrics are correct,
- flaky test details are visible.

---

# 140. Demo Repository

Build a companion repository:

```text
testgraph-demo-commerce/
```

Modules:

```text
auth
users
catalog
cart
orders
payments
notifications
inventory
common
```

Include:

- 200+ source files,
- 150+ tests,
- multiple dependency depths,
- several intentionally slow tests,
- one known flaky test,
- one circular dependency fixture,
- C++ library targets,
- JS frontend package if desired.

---

# 141. Demo Scenario

Change:

```text
payments/refund.cpp
```

Expected:

```text
changed files: 1
affected source files: N
selected tests: M
cache hits: K
executed tests: M-K
```

Show:

- dependency graph,
- execution plan,
- worker timeline,
- GitHub Check,
- dashboard metrics.

---

# 142. Benchmark Methodology

Compare:

```text
full suite
vs
impact-selected suite
vs
impact + cache
vs
impact + cache + smart scheduling
```

Measure:

```text
wall-clock duration
worker CPU seconds
tests executed
cache hits
selection accuracy
worker utilization
```

---

# 143. Resume Positioning

Example bullet after real measurements exist:

> Built **ChangeLens**, a C++ CI test-intelligence engine that constructs repository dependency graphs, performs transitive change-impact analysis, and selectively executes affected tests through a DAG-based parallel scheduler.

Second bullet:

> Implemented dependency-aware test-result caching, historical runtime-based worker balancing, flaky-test retries, GitHub Checks integration, and PostgreSQL/Redis-backed execution telemetry.

Third bullet:

> Reduced benchmark CI wall time by **X%** across **N** test runs while maintaining **Y% audited selection accuracy**, with explainable dependency paths for every selected test.

Only use verified benchmark numbers.

---

# 144. Interview Topics Generated by This Project

## C++

- RAII
- process management
- filesystem APIs
- concurrency
- synchronization
- smart pointers
- template abstractions
- memory ownership
- performance profiling

## Algorithms

- BFS / DFS
- shortest explanation paths
- Tarjan SCC
- topological scheduling
- critical path
- LPT scheduling
- hashing
- graph condensation

## Distributed Systems

- heartbeats
- worker failure
- job idempotency
- queue semantics
- stale run cancellation
- retries
- consistency
- durable vs ephemeral state

## CI/CD

- GitHub webhooks
- GitHub Checks
- pull-request lifecycle
- incremental execution
- audit mode
- full-suite fallback

## Caching

- cache keys
- transitive invalidation
- environment fingerprints
- remote cache
- correctness tradeoffs

## Reliability

- false-negative risk
- full-suite fallback
- confidence levels
- audit runs
- infrastructure failures

---

# 145. Important Engineering Tradeoffs

## Static Dependency Analysis

Advantages:

```text
fast
deterministic
works before historical coverage exists
```

Weaknesses:

```text
dynamic loading
reflection
runtime plugin systems
generated dependencies
```

## Runtime Coverage

Advantages:

```text
captures real execution
```

Weaknesses:

```text
requires instrumentation
can become stale
costs runtime
```

Best complete design:

```text
static graph
+
runtime coverage
+
safety rules
+
audit validation
```

---

# 146. What Not to Overengineer

Do not add technologies merely for resume keywords.

Do not require:

```text
Kafka
Kubernetes
machine learning
Elasticsearch
service mesh
```

unless a real scaling problem requires them.

The complete project is already technically rich with:

```text
C++
CMake
Clang tooling
Tree-sitter
libgit2
PostgreSQL
Redis
Docker
GitHub Apps
React
```

---

# 147. Recommended Development Order

Build the implementation in this order:

```text
1. C++ project skeleton
2. configuration
3. Git diff engine
4. C++ parser
5. CMake target graph
6. graph engine
7. reverse graph
8. SCC handling
9. test discovery
10. impact BFS
11. dependency path explanation
12. CLI
13. local process executor
14. historical durations
15. LPT scheduler
16. execution DAG
17. PostgreSQL
18. hashing
19. dependency-aware cache
20. flaky retry
21. GitHub App
22. GitHub Checks
23. React dashboard
24. Docker worker
25. remote worker protocol
26. worker recovery
27. Redis coordination
28. audit mode
29. runtime coverage
30. security hardening
31. benchmarking
32. documentation
```

---

# 148. Final Engineering Story

A developer modifies:

```text
src/payments/refund.cpp
```

ChangeLens receives the PR webhook.

It compares:

```text
base SHA
head SHA
```

The C++ analyzer updates the include graph and CMake target graph.

Reverse dependency traversal identifies the modules that transitively depend on the changed source.

ChangeLens finds:

```text
680 total tests
41 affected tests
```

Twelve tests have valid dependency-aware cache entries.

The remaining 29 tests are placed into an execution DAG.

Historical runtimes are used to distribute those tests across available workers.

One worker process fails; its job is safely requeued.

One test initially fails, then succeeds on retry, increasing its flaky score.

The final GitHub Check reports:

```text
680 repository tests
41 affected
12 cache hits
29 executed
28 passed
1 probable flaky

Full-suite estimate: 14m 32s
Actual runtime:      2m 06s
Estimated saved:    12m 26s
```

The dashboard shows:

- changed files,
- affected dependency graph,
- selected tests,
- reason for every selection,
- execution DAG,
- worker timeline,
- cache utilization,
- flaky-test history,
- failure signatures,
- CI time saved,
- audit accuracy.

Periodic audit runs execute the full suite to ensure ChangeLens has not missed relevant failures.

The final result is not merely a CI dashboard or test runner.

It is a **C++ dependency-analysis, scheduling, caching, and distributed execution system integrated into real pull-request workflows**.

---

# 149. Final Product Requirement

The core product must satisfy this statement:

> **Given two commits in a software repository, ChangeLens must safely determine the tests that should run, explain why they should run, reuse valid previous work, schedule remaining jobs efficiently across workers, recover from failures, report results to GitHub, and prove through audit runs that its optimization remains correct.**

That is the definition of the finished project.

