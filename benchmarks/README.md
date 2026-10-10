# Benchmarks

Generate a deterministic synthetic repository:

```bash
./build/testgraph-fixture-generator --files 10000 --tests 3000 --edges 50000 --out /tmp/testgraph-bench
./build/testgraph benchmark /tmp/testgraph-bench
```

Record graph build, incremental update, impact traversal, SCC/fingerprint calculation, scheduler planning and cache lookup throughput. Benchmark claims should include machine/compiler/build mode and must never be copied into product claims without actual measurements.
