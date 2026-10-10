# Architecture

The deployable product consists of a read-only C++ repository-analysis API and a React dashboard served by NGINX. One trusted repository is configured by the operator through `TESTGRAPH_REPOSITORY_PATH`; HTTP clients cannot choose arbitrary host directories.

`GET /api/v1/workspace` loads repository configuration, runs `RepositoryAnalyzer`, and serializes discovered nodes, edges, tests, unresolved imports, and effective policy. `POST /api/v1/impact` validates relative changed-file paths, performs a fresh scan, and calls `ImpactAnalyzer`. Selection explanation paths and configured fallback reasons are returned as JSON. File/test IDs are serialized as strings to preserve 64-bit identifiers in JavaScript.

The dashboard authenticates with a bearer token kept in memory, presents actual API results, and exposes no write or execution endpoints. NGINX serves the compiled assets, proxies API calls, applies security headers, and rate-limits analysis requests. Compose mounts the trusted repository read-only and provides the API token as an environment-backed secret.

The server validates configuration and scans the repository before opening its listener. Liveness and readiness are separate endpoints; readiness means startup validation completed, not a PostgreSQL/Redis connection check. SIGINT/SIGTERM stop request acceptance and exit after the active request finishes. Incoming headers and bodies are bounded, socket timeouts limit stalled clients, and complete-response writes tolerate disconnected clients.

The graph, impact, cache, scheduler, execution, coverage/audit, persistence, and worker modules remain transport-independent C++ components. Automated GitHub CI, runtime coverage collection, distributed workers, and durable execution history are future integrations; this release does not advertise them as live product features.
