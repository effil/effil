#!/bin/sh -e

brew update && true
brew install lua cmake && true

ROOT_DIR=$( cd $(dirname $0); pwd )
WORK_DIR=$ROOT_DIR/build

mkdir -p $WORK_DIR

# build
(cd $WORK_DIR && cmake $ROOT_DIR && make -j4 all && make install)

# run tests
(cd $WORK_DIR && ./tests)
(cd $ROOT_DIR/lua-tests && ./run_tests.lua)
