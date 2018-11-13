#!/usr/bin/env bash

set -e

FLAME_GRAPH="$(git rev-parse --show-toplevel)/libs/FlameGraph"

fail()
{
    echo "$@"
    exit 1
}

perl --version &> /dev/null || fail "Please install perl first"
report_file="perf.svg"

perf record -F 99 -g -- "$@"
perf script | "${FLAME_GRAPH}"/stackcollapse-perf.pl | "${FLAME_GRAPH}"/flamegraph.pl > "${report_file}"
echo "Generated ${report_file}"
