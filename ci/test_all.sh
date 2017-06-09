#!/usr/bin/env bash
set -e

for build_type in debug release; do
    mkdir -p $build_type
    (cd $build_type && cmake -DCMAKE_BUILD_TYPE=$build_type $@ .. && make -j4 all)
    (cd $build_type && ./tests && make stress_tests)
done
