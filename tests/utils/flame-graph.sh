#!/usr/bin/env bash

set -e

fail()
{
    echo "$@"
    exit 1
}

FLAME_GRAPH="$(git rev-parse --show-toplevel)/libs/FlameGraph"
REPORT_FILE="perf.svg"

perl --version &> /dev/null || fail "Please install perl to make it happen"
if [[ $(uname -s) != Linux* ]]; then
    fail "You can record profiles on Linux hosts only:("
fi


perf record -F 99 -g -- "$@"
perf script | "${FLAME_GRAPH}"/stackcollapse-perf.pl | "${FLAME_GRAPH}"/flamegraph.pl > "${REPORT_FILE}"
echo "Generated ${REPORT_FILE}"
