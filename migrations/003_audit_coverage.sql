BEGIN;
CREATE TABLE IF NOT EXISTS audit_runs (
  id BIGSERIAL PRIMARY KEY, run_id UUID NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
  optimized_status TEXT NOT NULL, full_suite_status TEXT NOT NULL, selection_miss_count INTEGER NOT NULL DEFAULT 0,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(), UNIQUE(run_id)
);
CREATE TABLE IF NOT EXISTS selection_misses (
  id BIGSERIAL PRIMARY KEY, audit_run_id BIGINT NOT NULL REFERENCES audit_runs(id) ON DELETE CASCADE,
  test_id BIGINT NOT NULL REFERENCES tests(id), failure_signature_id BIGINT REFERENCES failure_signatures(id),
  graph_snapshot_hash TEXT, parser_confidence TEXT, created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS coverage_map (
  test_id BIGINT NOT NULL REFERENCES tests(id) ON DELETE CASCADE,
  file_id BIGINT NOT NULL REFERENCES file_nodes(id) ON DELETE CASCADE,
  last_observed_at TIMESTAMPTZ NOT NULL DEFAULT now(), observation_count BIGINT NOT NULL DEFAULT 1,
  PRIMARY KEY(test_id,file_id)
);
CREATE TABLE IF NOT EXISTS repository_baselines (
  repository_id BIGINT PRIMARY KEY REFERENCES repositories(id) ON DELETE CASCADE,
  full_suite_duration_ms BIGINT NOT NULL, sample_count BIGINT NOT NULL DEFAULT 1, updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
COMMIT;
