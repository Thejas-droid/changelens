#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:-.}"
BUILD="${ROOT}/build"
cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD"
ctest --test-dir "$BUILD" --output-on-failure
"$BUILD/testgraph" analyze "$ROOT"
