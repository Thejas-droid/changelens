# Security

The deployed release analyzes one operator-configured, trusted repository. It does not run repository commands, accept uploads, or let requests select arbitrary host paths. Do not deploy it as a public multi-tenant service for untrusted inputs.

- Non-loopback API binding requires a token of at least 32 characters. API authorization uses a length check plus OpenSSL constant-time comparison.
- The dashboard keeps the token in memory; it is not persisted in localStorage, cookies, or build assets. Use HTTPS for remote access and protect the outer ingress with organizational access controls.
- Repository mounts are read-only. Docker socket mounts, unused databases, and queue/worker services are absent from the deployment.
- Containers run as unprivileged users with read-only root filesystems, dropped capabilities, no new privileges, process/memory limits, and temporary runtime storage.
- Changed-file paths must be relative, cannot contain parent traversal, and are bounded to 128 entries. Scanner file symlinks are skipped and dependency resolution does not traverse outside the repository.
- HTTP headers are capped at 32 KiB and bodies at 1 MiB. Duplicate headers, unsupported transfer encoding, invalid lengths, and incomplete bodies are rejected. Socket timeouts and a total read deadline bound stalled clients. NGINX rate-limits analysis calls.
- Client errors do not include exception details, absolute filesystem paths, or credentials. API responses are not cached; NGINX serves CSP, frame, MIME-sniffing, and referrer restrictions.
- `.env`, local credentials, agent folders, dependencies, and build outputs are excluded from Git and build contexts.

The core's process runner and webhook verification library are available for future integrations. Hardened job isolation, live GitHub Checks, per-user roles, multi-tenancy, and durable audit logging are not provided by this release.
