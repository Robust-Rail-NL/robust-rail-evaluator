#!/usr/bin/env bash
# Configure and build cTORS. Any arguments are passed through to cmake's
# configure step, e.g.:
#   ./build.sh -DCMAKE_BUILD_TYPE=Debug
#   ./build.sh -DCTORS_ASSERTIONS=ON -DTORS_VERSION_OVERRIDE=2.1.0
#
# Runs the actual compile with cmake --build's own parallelism (one job per
# core, minus a couple reserved so a local build doesn't stall the rest of
# the machine) and under `nice`, so it also backs off automatically whenever
# something else on the machine actually wants the CPU, rather than only
# ever leaving a fixed number of cores idle. Used both directly for local
# builds and from the Dockerfile, so the two never drift apart.
set -euo pipefail
cd "$(dirname "$0")"

NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
PARALLEL=$(( NPROC > 2 ? NPROC - 2 : 1 ))

mkdir -p build
cd build
cmake .. "$@"
nice -n 10 cmake --build . --parallel "$PARALLEL"
