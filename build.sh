#!/bin/sh
ROOT_DIR=$( cd $(dirname $0); pwd )
WORK_DIR=$ROOT_DIR/.work_dir

if ! [ -d $WORK_DIR ]; then
    mkdir $WORK_DIR
fi

cd $WORK_DIR
cmake $ROOT_DIR
make -j4
make install
