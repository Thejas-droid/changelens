cmake_minimum_required(VERSION 3.24)
project(ChangeLens VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TESTGRAPH_BUILD_TESTS "Build ChangeLens tests" ON)
option(TESTGRAPH_BUILD_SERVER "Build minimal HTTP server" ON)
option(TESTGRAPH_BUILD_WORKER "Build worker agent" ON)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

find_package(PostgreSQL QUIET)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h PATHS /usr/local/include /usr/include)
find_library(HIREDIS_LIBRARY hiredis PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu)

add_library(testgraph_core
  src/build/build_graph.cpp
  src/cache/cache.cpp
  src/config/config.cpp
  src/executor/process_executor.cpp
  src/flaky/flaky.cpp
  src/git/git_client.cpp
  src/graph/dependency_graph.cpp
  src/impact/impact_analyzer.cpp
  src/metrics/metrics.cpp
  src/parser/parser.cpp
  src/repository/repository_analyzer.cpp
  src/run/run_coordinator.cpp
  src/scheduler/scheduler.cpp
  src/security/webhook_verifier.cpp
  src/test/test_discovery.cpp
  src/worker/worker_registry.cpp
  src/api/workspace.cpp
  src/api/simple_http_server.cpp
  src/audit/audit.cpp
  src/coverage/coverage_map.cpp
  src/statistics/test_statistics.cpp
  src/db/persistence.cpp
  src/github/github_service.cpp
)

target_include_directories(testgraph_core PUBLIC include)
target_link_libraries(testgraph_core PUBLIC OpenSSL::Crypto Threads::Threads)
target_compile_options(testgraph_core PRIVATE -Wall -Wextra -Wpedantic)

if(PostgreSQL_FOUND)
  target_sources(testgraph_core PRIVATE src/db/postgres_store.cpp)
  target_include_directories(testgraph_core PRIVATE ${PostgreSQL_INCLUDE_DIRS})
  target_link_libraries(testgraph_core PUBLIC PostgreSQL::PostgreSQL)
  target_compile_definitions(testgraph_core PUBLIC TESTGRAPH_HAS_POSTGRES=1)
endif()
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  target_sources(testgraph_core PRIVATE src/worker/redis_coordinator.cpp)
  target_include_directories(testgraph_core PRIVATE ${HIREDIS_INCLUDE_DIR})
  target_link_libraries(testgraph_core PUBLIC ${HIREDIS_LIBRARY})
  target_compile_definitions(testgraph_core PUBLIC TESTGRAPH_HAS_REDIS=1)
endif()

add_executable(testgraph apps/cli/main.cpp)
target_link_libraries(testgraph PRIVATE testgraph_core)

if(TESTGRAPH_BUILD_SERVER)
  add_executable(testgraph-server apps/server/main.cpp)
  target_link_libraries(testgraph-server PRIVATE testgraph_core)
endif()

if(TESTGRAPH_BUILD_WORKER)
  add_executable(testgraph-worker apps/worker/main.cpp)
  target_link_libraries(testgraph-worker PRIVATE testgraph_core)
endif()

if(TESTGRAPH_BUILD_TESTS)
  enable_testing()
  add_executable(testgraph-unit-tests tests/unit/test_main.cpp)
  target_link_libraries(testgraph-unit-tests PRIVATE testgraph_core)
  add_test(NAME testgraph-unit COMMAND testgraph-unit-tests)

  add_executable(testgraph-integration-tests tests/integration/test_integration.cpp)
  target_link_libraries(testgraph-integration-tests PRIVATE testgraph_core)
  add_test(NAME testgraph-integration COMMAND testgraph-integration-tests)
endif()

add_executable(testgraph-fixture-generator tools/fixture-generator/main.cpp)

