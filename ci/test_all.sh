#!/usr/bin/env bash
set -e

if [ -z "$LUA_BIN" ]; then
    LUA_BIN="lua"
fi

for build_type in debug release; do
    cmake -H. -B$build_type  -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=$build_type $@
    cmake --build $build_type --config Release

    (cd $build_type && STRESS=1 $LUA_BIN ../tests/lua/run_tests)
done
