BEGIN;
CREATE TABLE IF NOT EXISTS build_targets (
  id BIGSERIAL PRIMARY KEY, repository_id BIGINT NOT NULL REFERENCES repositories(id) ON DELETE CASCADE,
  name TEXT NOT NULL, target_type TEXT NOT NULL, build_directory TEXT, UNIQUE(repository_id,name)
);
CREATE TABLE IF NOT EXISTS target_files (
  target_id BIGINT NOT NULL REFERENCES build_targets(id) ON DELETE CASCADE,
  file_node_id BIGINT NOT NULL REFERENCES file_nodes(id) ON DELETE CASCADE,
  PRIMARY KEY(target_id,file_node_id)
);
CREATE TABLE IF NOT EXISTS target_dependencies (
  source_target_id BIGINT NOT NULL REFERENCES build_targets(id) ON DELETE CASCADE,
  target_target_id BIGINT NOT NULL REFERENCES build_targets(id) ON DELETE CASCADE,
  PRIMARY KEY(source_target_id,target_target_id)
);
COMMIT;
