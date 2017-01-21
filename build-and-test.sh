#!/bin/sh -e
ROOT_DIR=$( cd $(dirname $0); pwd )
BUILD_DIR=$ROOT_DIR/build
mkdir -p $BUILD_DIR

cd $BUILD_DIR
cmake $ROOT_DIR

make -j4 all
make install
./tests

cd $ROOT_DIR/lua-tests
./run_tests.lua