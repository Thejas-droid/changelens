# Cache

Cache material includes test identity, dependency fingerprint, command, runner/runtime versions, build configuration, environment fingerprint and tool schema version. It is SHA-256 hashed. Only passing results are reusable.

Transitive invalidation happens naturally because a changed content hash alters the SCC/component fingerprint of every dependent node. Runtime/compiler/lockfile/config/environment changes must also alter key material.
