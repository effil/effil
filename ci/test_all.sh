#!/usr/bin/env bash
set -e

if [ -z "$LUA_BIN" ]; then
    LUA_BIN="lua"
fi

for build_type in debug release; do
    mkdir -p $build_type
    (cd $build_type && cmake -DCMAKE_BUILD_TYPE=$build_type $@ .. && make -j4 install)

    # FIXME: creation of sol::state with luajit in c++ tests 
    # leads to memory corruption segmentation fault
    # this is temporary workaround
    if [ -z "$SKIP_CPP_TESTS" ]; then
        (cd $build_type && ./tests)
    else
        echo "C++ tests skipped!"
    fi

    (cd $build_type && $LUA_BIN tests.lua)
done
