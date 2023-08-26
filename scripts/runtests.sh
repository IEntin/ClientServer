#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

if [[ $@ == "--help" || $@ == "-h" || $# -ne 1 ]]
then 
    echo "Usage: [path]/runtests.sh 2>&1 <number to repeat> | tee testslog.txt"
    exit 0
fi 

trap SIGHUP SIGINT SIGTERM

make -j4 testbin
$UP_DIR/testbin --gtest_repeat=$1 --gtest_break_on_failure
