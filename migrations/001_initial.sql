BEGIN;
CREATE TABLE IF NOT EXISTS repositories (
  id BIGSERIAL PRIMARY KEY,
  github_repository_id BIGINT UNIQUE,
  owner TEXT NOT NULL,
  name TEXT NOT NULL,
  default_branch TEXT NOT NULL DEFAULT 'main',
  installation_id TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  UNIQUE(owner,name)
);
CREATE TABLE IF NOT EXISTS commits (
  id BIGSERIAL PRIMARY KEY, repository_id BIGINT NOT NULL REFERENCES repositories(id) ON DELETE CASCADE,
  sha TEXT NOT NULL, author TEXT, message TEXT, created_at TIMESTAMPTZ NOT NULL DEFAULT now(), UNIQUE(repository_id,sha)
);
CREATE TABLE IF NOT EXISTS pull_requests (
  id BIGSERIAL PRIMARY KEY, repository_id BIGINT NOT NULL REFERENCES repositories(id) ON DELETE CASCADE,
  number INTEGER NOT NULL, base_sha TEXT NOT NULL, head_sha TEXT NOT NULL, state TEXT NOT NULL DEFAULT 'open',
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now(), UNIQUE(repository_id,number)
);
CREATE TABLE IF NOT EXISTS runs (
  id UUID PRIMARY KEY, repository_id BIGINT NOT NULL REFERENCES repositories(id) ON DELETE CASCADE,
  pull_request_number INTEGER, base_sha TEXT NOT NULL, head_sha TEXT NOT NULL, status TEXT NOT NULL,
  analysis_confidence TEXT NOT NULL, full_suite_fallback BOOLEAN NOT NULL DEFAULT FALSE,
  started_at TIMESTAMPTZ NOT NULL DEFAULT now(), completed_at TIMESTAMPTZ, cancelled_at TIMESTAMPTZ
);
CREATE TABLE IF NOT EXISTS file_nodes (
  id BIGSERIAL PRIMARY KEY, repository_id BIGINT NOT NULL REFERENCES repositories(id) ON DELETE CASCADE,
  path TEXT NOT NULL, kind TEXT NOT NULL, language TEXT, content_hash TEXT NOT NULL, dependency_fingerprint TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(), updated_at TIMESTAMPTZ NOT NULL DEFAULT now(), UNIQUE(repository_id,path)
);
CREATE TABLE IF NOT EXISTS dependency_edges (
  id BIGSERIAL PRIMARY KEY, repository_id BIGINT NOT NULL REFERENCES repositories(id) ON DELETE CASCADE,
  source_file_id BIGINT NOT NULL REFERENCES file_nodes(id) ON DELETE CASCADE,
  target_file_id BIGINT NOT NULL REFERENCES file_nodes(id) ON DELETE CASCADE,
  dependency_type TEXT NOT NULL, created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  UNIQUE(repository_id,source_file_id,target_file_id,dependency_type)
);
CREATE TABLE IF NOT EXISTS tests (
  id BIGSERIAL PRIMARY KEY, repository_id BIGINT NOT NULL REFERENCES repositories(id) ON DELETE CASCADE,
  file_node_id BIGINT NOT NULL REFERENCES file_nodes(id) ON DELETE CASCADE,
  framework TEXT NOT NULL, suite_name TEXT, test_name TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(), updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  UNIQUE(repository_id,file_node_id,framework,suite_name,test_name)
);
CREATE TABLE IF NOT EXISTS jobs (
  id BIGSERIAL PRIMARY KEY, run_id UUID NOT NULL REFERENCES runs(id) ON DELETE CASCADE, job_type TEXT NOT NULL,
  status TEXT NOT NULL, estimated_duration_ms BIGINT NOT NULL DEFAULT 0, actual_duration_ms BIGINT,
  attempt INTEGER NOT NULL DEFAULT 1, worker_id TEXT, execution_token TEXT,
  started_at TIMESTAMPTZ, completed_at TIMESTAMPTZ, UNIQUE(id,attempt,execution_token)
);
CREATE TABLE IF NOT EXISTS job_dependencies (
  job_id BIGINT NOT NULL REFERENCES jobs(id) ON DELETE CASCADE,
  depends_on_job_id BIGINT NOT NULL REFERENCES jobs(id) ON DELETE CASCADE,
  PRIMARY KEY(job_id,depends_on_job_id)
);
CREATE TABLE IF NOT EXISTS workers (
  id TEXT PRIMARY KEY, state TEXT NOT NULL, last_heartbeat_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  current_job_id BIGINT REFERENCES jobs(id), available_cpu DOUBLE PRECISION, available_memory BIGINT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS failure_signatures (
  id BIGSERIAL PRIMARY KEY, signature_hash TEXT NOT NULL UNIQUE, normalized_message TEXT,
  first_seen_at TIMESTAMPTZ NOT NULL DEFAULT now(), last_seen_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS test_executions (
  id BIGSERIAL PRIMARY KEY, run_id UUID NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
  test_id BIGINT NOT NULL REFERENCES tests(id) ON DELETE CASCADE, job_id BIGINT REFERENCES jobs(id),
  worker_id TEXT, status TEXT NOT NULL, duration_ms BIGINT NOT NULL DEFAULT 0, attempt INTEGER NOT NULL,
  cache_hit BOOLEAN NOT NULL DEFAULT FALSE, failure_signature_id BIGINT REFERENCES failure_signatures(id),
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(), UNIQUE(job_id,test_id,attempt)
);
CREATE TABLE IF NOT EXISTS webhook_deliveries (
  delivery_id TEXT PRIMARY KEY, event_type TEXT NOT NULL, received_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
COMMIT;
