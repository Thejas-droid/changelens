# Workers

The worker registry models REGISTERING, IDLE, BUSY, DRAINING and DEAD states. Heartbeats update current job and capacity. When a heartbeat expires, the registry marks the worker dead and returns the in-flight job IDs for safe requeue.

Every completion is keyed by `(job id, attempt, execution token)`. Duplicate messages are ignored by the completion gate; durable deployments should enforce the same property transactionally with a unique constraint.
