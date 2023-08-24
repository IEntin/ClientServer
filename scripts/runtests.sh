#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ $@ == "--help" || $@ == "-h" || $# -ne 1 ]]
then 
    echo "Usage: ./runtests.sh 2>&1 50 | tee testslog.txt"
    exit 0
fi 

trap SIGHUP SIGINT SIGTERM

make -j4 testbin
./testbin --gtest_repeat=$1 --gtest_break_on_failure
