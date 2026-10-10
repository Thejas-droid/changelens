# Scheduler

`JobGraph` validates DAG structure with Kahn topological ordering. Critical path weight is `duration(node) + max(child weight)`. For independent ready work, ChangeLens uses longest-processing-time-first and assigns each next job to the worker with the smallest predicted load.

The reference planner returns predicted start/finish timestamps for each worker assignment. Runtime estimates can come from the rolling successful-sample estimator and can evolve to EWMA without changing scheduler APIs.
