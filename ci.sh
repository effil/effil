#!/bin/sh -e

sudo apt-get update -qq
sudo apt-get install -y cmake liblua5.2-dev

ROOT_DIR=$( cd $(dirname $0); pwd )
WORK_DIR=$ROOT_DIR/build

mkdir -p $WORK_DIR

# build
(cd $WORK_DIR && cmake $ROOT_DIR && make -j4 all && make install)

# run tests
(cd $WORK_DIR && ./tests)
(cd $ROOT_DIR/lua-tests && ./run_tests.lua)
