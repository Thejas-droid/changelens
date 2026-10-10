#include "testgraph/executor/process_executor.hpp"
#include "testgraph/worker/worker_registry.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

using namespace testgraph;
int main(int argc, char** argv) {
  const std::string id = std::getenv("TESTGRAPH_WORKER_ID") ? std::getenv("TESTGRAPH_WORKER_ID") : "worker-local";
  WorkerRegistry registry;
  auto info = registry.registerWorker(id, 1.0, 0);
  std::cout << "registered worker " << info.id << " state=IDLE\n";
  if (argc > 1 && std::string(argv[1]) == "--exec") {
    if (argc < 3) return 2;
    ProcessSpec spec; spec.workingDirectory = ".";
    for (int i = 2; i < argc; ++i) spec.argv.emplace_back(argv[i]);
    registry.heartbeat(id, 1, 0.0, 0);
    auto r = ProcessExecutor().run(spec);
    registry.heartbeat(id, std::nullopt, 1.0, 0);
    std::cout << r.stdoutText; std::cerr << r.stderrText;
    return r.exitCode;
  }
  std::cout << "No coordinator URL configured; use --exec for local execution or Docker Compose for the full stack.\n";
  return 0;
}
