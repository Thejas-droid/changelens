# ChangeLens

ChangeLens is a C++20 change-impact analysis service with a React/TypeScript dashboard. It maps repository dependencies and traces how specified source changes affect files and tests. Affected-test selection is one application of its dependency analysis. All dashboard results come from the configured repository through the API.

The product name is **ChangeLens**. Existing `testgraph` commands, configuration filenames, C++ namespaces, and `TESTGRAPH_*` environment variables retain their names for compatibility.

## Release scope

This release supports repository overview, dependency exploration, test search, changed-file impact selection, and read-only configuration inspection. It is designed for a trusted repository in an internal deployment. It does not execute repository code, accept arbitrary repository uploads, launch CI jobs from webhooks, or provide distributed execution history.

The core library also contains scheduling, caching, process execution, flake tracking, coverage/audit models, and optional PostgreSQL/Redis adapters. Those execution/infrastructure features are not connected to this dashboard release.

## Run with Docker Compose

Requirements: Docker Engine/Desktop with a recent Compose v2 supporting environment-backed secrets.

```bash
cp .env.example .env
```

Set `TESTGRAPH_API_TOKEN` in `.env` to a unique token generated with `openssl rand -hex 32`. Set `TESTGRAPH_REPOSITORY_PATH` to an absolute path to the trusted repository to analyze. Then:

```bash
docker compose up --build -d --wait
```

Open http://localhost:3000 and enter the access token. The repository is mounted read-only; only the dashboard port is published, on loopback. The API token is mounted into the server as a Compose secret. No database, queue, Docker socket, or external credentials are needed for this release.

For remote access, place the dashboard behind your organization's HTTPS reverse proxy and access controls. See [deployment and operations](docs/operations.md) and [security](docs/security.md).

## Local development

Linux/WSL requirements: CMake 3.24+, a C++20 compiler, Ninja, OpenSSL development files, Git, Python 3, and Node.js 22.12+ or 24+.

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build libssl-dev git python3
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/integration/test_http.py ./build/testgraph-server
```

Start the API from the repository root:

```bash
TESTGRAPH_REPOSITORY_PATH=/absolute/path/to/repository ./build/testgraph-server
```

The server defaults to `127.0.0.1:8080`. A local loopback bind may run without an access token; set `TESTGRAPH_API_TOKEN` to require authentication. Non-loopback binding requires a token of at least 32 characters.

In another terminal:

```bash
cd web/dashboard
npm ci
npm run dev
```

Open the URL Vite prints. The dashboard proxies API calls to `127.0.0.1:8080`. If running locally without a token, set `TESTGRAPH_APP_ORIGIN` on the server to the exact dashboard origin (for example `http://localhost:5173`) before submitting impact requests. With a token enabled, requests use bearer authentication.

In WSL, use Linux Node/npm. Stop any Windows dev server using the same dependency folder before switching platforms; locked native executables can prevent dependency installation.

## Dashboard

- **Overview:** file/test counts, resolved edges, parser confidence, unresolved imports, and scan timestamps.
- **Impact analysis:** submit up to 128 repository-relative changed files; inspect selected tests, fallback reasons, and dependency explanation paths.
- **Dependency explorer:** search real file nodes and inspect a file's direct dependencies/dependents. Graph views cap at 80 nodes to keep the browser responsive.
- **Tests:** search discovered tests and filter by framework.
- **Configuration:** inspect effective safety patterns and library defaults. Update repository configuration outside the dashboard, then refresh.

Impact analysis selects tests; it does not execute them. Static parsers have limitations; confidence and fallback policies should be reviewed before using selections in CI.

## API

| Endpoint | Behavior |
| --- | --- |
| `GET /health/live` | Process liveness; unauthenticated |
| `GET /health/ready` | Configured repository was successfully scanned at startup; unauthenticated |
| `GET /api/v1/workspace` | Fresh repository scan, test inventory, graph, configuration |
| `POST /api/v1/impact` | Fresh scan and impact selection; newline-delimited paths in a `text/plain` body |
| `GET /metrics` | Authenticated API request counter in Prometheus format |
| `POST /webhooks/github` | Returns 501; automated webhook execution is not part of this release |

API routes require `Authorization: Bearer <token>` when a token is configured. Tokens are held in browser memory and are never persisted by the dashboard. The server handles requests sequentially; use the reverse proxy rate limit and avoid very large repositories without capacity testing.

## CLI

```bash
./build/testgraph analyze /path/to/repository
./build/testgraph graph /path/to/repository > graph.dot
./build/testgraph affected BASE HEAD /path/to/repository
./build/testgraph explain tests/example_test.cpp BASE HEAD /path/to/repository
```

The Git-based commands require valid revisions in a real checkout. The dashboard's changed-file input works independently of Git history.

## Verification and contributing

CI builds/tests the C++ core, runs HTTP integration tests, builds the dashboard with the lockfile, and builds/smoke-tests the Compose deployment. See [dashboard development](web/dashboard/README.md) and [architecture](docs/architecture.md).

Do not commit `.env`, access tokens, dependency directories, compiled outputs, or local agent settings. This repository's ignore files cover those artifacts. Configure your own Git remote before pushing.
