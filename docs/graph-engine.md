# Graph Engine

A dependency edge `A -> B` means A depends on B. Reverse adjacency is maintained at insertion time so changed-file impact is a BFS over dependents. Traversal uses visited sets, so cycles are safe.

Tarjan SCCs are calculated for diagnostics and stable fingerprints. Each SCC is hashed from sorted member content hashes plus sorted external component fingerprints. Every node in the component receives the same dependency fingerprint, preventing recursive hashing loops.

For explainability, selection paths are reconstructed from a selected test toward a changed dependency. For C++, the file graph can be combined with `BuildGraph`, which models source membership, target links and test targets.
