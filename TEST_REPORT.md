# ChangeLens release verification

Verification date: 2026-10-10.

## Environment

Linux/WSL, GCC 13.3.0, CMake 3.28.3, C++20/OpenSSL, Node.js 22.14.0, and headless Chromium. Temporary NGINX 1.24 was used to validate the reverse-proxy directives on the host; the deployment image specifies NGINX 1.30.5.

## Passed checks

- Release C++ build and both existing CTest suites.
- `tests/integration/test_http.py`: actual scan/test discovery, dependency-based selection, fallback selection, authorization rejection, traversal/absolute/empty/oversized path rejection, symlink exclusion, nested dependency-directory exclusion, JSON escaping, duplicate/incomplete/oversized HTTP requests, unsupported transfer encoding, disconnected clients, graceful termination, and fail-closed non-loopback startup.
- Frontend TypeScript check and Vite production bundle, including the ChangeLens rename and classic light theme.
- Headless browser against the actual API: wrong/correct token handling, counts matching the exact displayed response, dependency selection and full-suite fallback, invalid paths, graph neighborhoods, test search and empty state, read-only policy, and credentials cleared on reload.
- Renamed product identity verified in the page title and navigation. Desktop and mobile screenshots reviewed.
- All five product pages checked at a 390px viewport without document overflow. No uncaught browser errors during the tested flows.
- Automated axe checks with WCAG A/AA tags found no violations in the tested default states of the five pages. This is not a full accessibility certification.
- NGINX configuration validation and local reverse-proxy smoke checks: asset delivery, security headers, unauthorized API rejection, and bearer forwarding to actual repository analysis.
- Compose YAML/service-policy checks: read-only runtimes, restricted loopback ingress, no Docker socket, no unused infrastructure, and npm manifest/lockfile consistency.
- Independent project Git repository initialized; secrets/generated artifacts excluded, while core `src/build` and `include/testgraph/build` source folders remain included.
- Clean export of only Git-included files configured, compiled, and passed both CTest suites; source files needed for a fresh checkout are included.

## Release gate still requiring Docker

Docker Engine is unavailable in the local execution environment. The actual Docker image builds and Compose runtime smoke test were not run here. The GitHub Actions workflow includes those checks; they must pass before deployment. A parsed Compose file and host NGINX smoke test do not substitute for that gate.

The product is scoped to internal, trusted-repository analysis. Distributed CI execution, GitHub Checks, runtime coverage collectors, and durable execution history are not part of this release. Capacity/security testing for large repositories and the target ingress remains deployment-specific.
