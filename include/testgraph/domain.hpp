#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace testgraph {
using FileId = std::uint64_t;
using TestId = std::uint64_t;
using JobId = std::uint64_t;
using WorkerId = std::string;

constexpr FileId kInvalidFileId = 0;

enum class FileKind { Source, Test, Config, Build, Unknown };
enum class DependencyType { Import, Include, RuntimeObserved, BuildDependency };
enum class RunStatus { Queued, Analyzing, Planning, Running, Passed, Failed, Warning, Cancelled, Error };
enum class Confidence { High, Medium, Low, Unsafe };
enum class JobType { Checkout, Setup, Build, StaticAnalysis, UnitTest, IntegrationTest, E2ETest, PostProcess, Report };
enum class JobStatus { Queued, Ready, Running, Passed, Failed, Skipped, Cancelled, Error };
enum class WorkerState { Registering, Idle, Busy, Draining, Dead };
enum class TestStatus { Pass, Fail, Error, Skipped };

struct FileNode {
  FileId id{};
  std::filesystem::path path;
  FileKind kind{FileKind::Unknown};
  std::string language;
  std::string contentHash;
  std::string dependencyFingerprint;
};

struct DependencyEdge {
  FileId from{};
  FileId to{};
  DependencyType type{DependencyType::Import};
};

struct TestCase {
  TestId id{};
  FileId fileId{};
  std::filesystem::path filePath;
  std::string framework;
  std::string suiteName;
  std::string testName;
  std::vector<std::string> command;
};

struct ChangedFile {
  enum class Status { Added, Modified, Deleted, Renamed };
  Status status{Status::Modified};
  std::filesystem::path path;
  std::filesystem::path oldPath;
};

struct ParseResult {
  std::vector<std::string> dependencies;
  std::vector<std::string> unresolved;
};

struct ImpactExplanation {
  TestId testId{};
  std::vector<FileId> path; // test -> ... -> changed
};

struct ImpactResult {
  std::vector<FileId> changed;
  std::vector<FileId> affected;
  std::vector<TestId> selectedTests;
  std::vector<ImpactExplanation> explanations;
  Confidence confidence{Confidence::High};
  bool fullSuiteFallback{false};
  std::vector<std::string> reasons;
};

struct ProcessSpec {
  std::vector<std::string> argv;
  std::filesystem::path workingDirectory;
  std::unordered_map<std::string, std::string> environment;
  std::chrono::milliseconds timeout{std::chrono::minutes(15)};
};

struct ProcessResult {
  int exitCode{-1};
  bool timedOut{false};
  bool cancelled{false};
  std::string stdoutText;
  std::string stderrText;
  std::chrono::milliseconds duration{0};
};

struct TestExecutionResult {
  TestStatus status{TestStatus::Error};
  std::chrono::milliseconds duration{0};
  std::string output;
  std::string failureSignature;
};

struct JobNode {
  JobId id{};
  JobType type{JobType::UnitTest};
  std::vector<JobId> dependencies;
  std::vector<JobId> dependents;
  std::chrono::milliseconds estimatedDuration{0};
  int priority{0};
  JobStatus status{JobStatus::Queued};
  std::optional<TestId> testId;
  std::vector<std::string> command;
};

inline std::string toString(Confidence c) {
  switch (c) {
    case Confidence::High: return "HIGH";
    case Confidence::Medium: return "MEDIUM";
    case Confidence::Low: return "LOW";
    case Confidence::Unsafe: return "UNSAFE";
  }
  return "UNSAFE";
}

inline std::string toString(FileKind k) {
  switch (k) {
    case FileKind::Source: return "SOURCE";
    case FileKind::Test: return "TEST";
    case FileKind::Config: return "CONFIG";
    case FileKind::Build: return "BUILD";
    case FileKind::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}
} // namespace testgraph
