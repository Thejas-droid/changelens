# Implementation status

## Deployable release: repository analysis

The dashboard and API are connected to actual repository scanning and impact analysis. Supported product surfaces are overview, test inventory/search, dependency exploration, changed-file impact selection with explanations/fallbacks, and read-only configuration inspection.

Deployment uses a read-only source mount, bearer-protected API, unprivileged server/NGINX containers, bounded HTTP input, real startup validation, and a lockfile-based dashboard build. CI includes C++ tests, HTTP integration, UI build, container builds, and Compose health/API smoke checks.

## Core library capabilities

Implemented modules include dependency/reverse graphs, cycle-safe traversal, Tarjan SCCs, fingerprints, parser adapters, test discovery, Git diff mapping, CMake target scanning, impact/fallback policies, process execution, local run coordination, DAG/LPT scheduling, passing-result cache, failure/flake models, coverage/audit models, worker lifecycle models, webhook HMAC verification, and optional PostgreSQL/Redis adapters.

## Integration boundaries

Automated GitHub webhook execution, live Checks API updates, distributed job polling, runtime coverage collectors, and durable execution history are not part of the deployed analysis product. The webhook route returns 501 rather than acknowledging unprocessed events. There are no synthetic CI metrics or worker/run records in the product UI.

A release should be described as an internal repository-analysis dashboard. Broader CI automation and untrusted multi-tenant hosting require additional implementation and validation.
