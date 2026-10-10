# ChangeLens dashboard

A repository-analysis interface backed by the C++ service. It has no sample scenarios or simulated statistics.

## Visual style

The dashboard uses a light neutral workspace, white panels, navy actions, compact summary rows, and restrained borders. System fonts keep the interface consistent with desktop tools and avoid external font requests. Green and amber are reserved for analysis status; visible keyboard focus and responsive layouts are retained.

## Develop

Use Linux Node/npm in WSL, or a consistent native Windows toolchain. Node.js 22.12+ or 24+ is supported.

```bash
npm ci
npm run dev
npm run build
```

If WSL does not detect edits on a Windows-mounted drive, run `CHOKIDAR_USEPOLLING=true npm run dev` to enable polling.

The development proxy targets `127.0.0.1:8080`. Start `testgraph-server` with `TESTGRAPH_REPOSITORY_PATH` pointing at your trusted repository. If `TESTGRAPH_API_TOKEN` is configured, enter it on the connection screen; credentials are kept in memory only. A tokenless local server validates POST origins against `TESTGRAPH_APP_ORIGIN` (set it to the exact Vite URL).

## Product behavior

Overview, impact selection, graphs, tests, and configuration display actual API responses. A failed request shows an error and retry action. Refresh preserves previously loaded data while scanning, and reports when that data may be stale. Authentication failures clear repository data and request a token. The UI never substitutes sample data.

Impact analysis submits newline-delimited relative paths to the server. Results distinguish selection from execution and show dependency paths or fallback selection reasons. Graph views display at most 80 file nodes; filtering and file neighborhoods constrain large graphs. The file inspector offers the same direct dependency relationships in text form.

Configuration is read-only. Edit `testgraph.yml` in the configured repository and refresh. The dashboard does not execute tests or modify source files.

## Deployment

The Docker image builds with `npm ci` and serves assets through unprivileged NGINX on port 8080. The main Compose file connects its `/api/` proxy to the analysis server and exposes the dashboard at http://localhost:3000. Its runtime filesystem is read-only with temporary NGINX state in `/tmp`.

## Manual checks

- Authentication: protected API rejects missing/incorrect tokens, Connect succeeds with the deployment token, and reload does not retain credentials.
- Overview: counts match `testgraph analyze` on the same repository; refresh updates the timestamp.
- Impact: known dependency changes select the expected tests; fallback patterns select the full suite; invalid/empty/parent paths are rejected; request failures clear obsolete results.
- Explorer: search and neighborhoods reflect real edges; the inspector lists direct dependencies and dependents.
- Tests: search/framework filters update results and provide an empty state.
- Configuration: safety patterns match repository configuration.
- Keyboard/mobile: controls have visible focus; Skip to content preserves the route; tables scroll within the viewport; no page overflow at phone width.
