#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ $@ == "--help" || $@ == "-h" || $# -ne 1 ]]
then 
    echo "Usage: ./runtests.sh 2>&1 50 | tee testslog.txt"
    exit 0
fi 

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

trap SIGHUP SIGINT SIGTERM

make -j4 testbin
$SCRIPT_DIR/testbin --gtest_repeat=$1 --gtest_break_on_failure
