#!/usr/bin/env bash
#FIXME:
#set -e

PADDING="===================="
GRADLE="./gradlew --no-daemon  --console plain"

for lua_version in 5.2 5.3; do
    for build_type in debug release; do
        echo $PADDING
        echo "lua version: $lua_version"
        echo "build type: $build_type"
        echo $PADDING
        $GRADLE clean
        $GRADLE cpptests_$build_type luatests_$build_type -PUSE_LUA=$lua_version
        build/$build_type/tests       # Run C++ tests
        (cd build && ./lua tests.lua) # Run lua tests
    done
done