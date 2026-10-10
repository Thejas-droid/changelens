BEGIN;
CREATE TABLE IF NOT EXISTS test_statistics (
  test_id BIGINT PRIMARY KEY REFERENCES tests(id) ON DELETE CASCADE,
  execution_count BIGINT NOT NULL DEFAULT 0, pass_count BIGINT NOT NULL DEFAULT 0,
  failure_count BIGINT NOT NULL DEFAULT 0, retry_success_count BIGINT NOT NULL DEFAULT 0,
  average_duration_ms DOUBLE PRECISION NOT NULL DEFAULT 0, p50_duration_ms BIGINT NOT NULL DEFAULT 0,
  p95_duration_ms BIGINT NOT NULL DEFAULT 0, flake_score DOUBLE PRECISION NOT NULL DEFAULT 0,
  last_execution_at TIMESTAMPTZ, updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS cache_entries (
  cache_key TEXT PRIMARY KEY, test_id BIGINT REFERENCES tests(id) ON DELETE CASCADE,
  status TEXT NOT NULL CHECK(status='PASS'), duration_ms BIGINT NOT NULL DEFAULT 0,
  dependency_fingerprint TEXT NOT NULL, environment_fingerprint TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(), expires_at TIMESTAMPTZ NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_runs_repo_started ON runs(repository_id,started_at DESC);
CREATE INDEX IF NOT EXISTS idx_exec_test_created ON test_executions(test_id,created_at DESC);
CREATE INDEX IF NOT EXISTS idx_edges_target ON dependency_edges(repository_id,target_file_id);
CREATE INDEX IF NOT EXISTS idx_jobs_run_status ON jobs(run_id,status);
COMMIT;
