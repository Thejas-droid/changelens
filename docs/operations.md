# Deployment and operations

## Scope

Deploy one trusted repository per analysis service. The mounted source is read-only and is never executed by the API. The current HTTP server processes requests sequentially and rescans for each workspace/impact request. Capacity-test large repositories and use the bundled reverse proxy rate limit; this release is not a multi-tenant public code-analysis service.

## Start

Copy `.env.example` to `.env`. Set a unique `TESTGRAPH_API_TOKEN` (at least 32 characters; `openssl rand -hex 32` generates a suitable value) and an absolute `TESTGRAPH_REPOSITORY_PATH`. Ensure the unprivileged container user can read the source and configuration.

```bash
docker compose config --quiet
docker compose up --build -d --wait
```

Open http://localhost:3000, enter the token, and confirm the repository name/counts. The dashboard port is bound to loopback. For access from other machines, use a managed HTTPS reverse proxy with your organization's identity/access controls; explicitly configure the outer ingress rather than exposing the raw API port.

## Health and shutdown

- `/health/live`: the process responds.
- `/health/ready`: the configured directory and repository scan were validated before listening. It does not imply ongoing external-service connectivity.
- `/metrics`: bearer-protected API request counter.
- SIGINT/SIGTERM stops accepting new work and exits after an active scan/request completes. Scanning is synchronous; an unusually large scan may exceed an orchestrator's stop grace period.

```bash
docker compose logs --tail 100
docker compose down
```

The service stores no execution history. There is no database backup or migration task for this release. Repository content and configuration are maintained by the operator outside the service.

## Rotate access

Update the token in `.env` or your secret manager and recreate the server container. Browser users must reconnect. Do not put the token into frontend build variables, URLs, Git, or logs. Compose environment-backed secrets require a recent Compose v2; they are not Swarm deployment secrets.

## Upgrade and validation

CI runs the core tests, API integration suite, dashboard build, Compose configuration validation, image builds, and a real deployment smoke check. Run those checks before release. Docker image tags are versioned where applicable; resolve and pin verified image digests in your deployment inventory if immutable image provenance is required.

## Troubleshooting

An invalid root, unreadable configuration, missing secret, weak token on an exposed bind, or unavailable port causes startup failure. An HTTP analysis error reports a generic message to clients; inspect the configured repository and server logs. Static parsers cannot resolve every runtime dependency; review confidence and unresolved imports before applying selective testing in CI.

References: [Docker Compose service controls](https://docs.docker.com/reference/compose-file/services/), [environment-backed secrets](https://docs.docker.com/reference/compose-file/secrets/), and [NGINX reverse proxy configuration](https://nginx.org/en/docs/http/ngx_http_proxy_module.html).
