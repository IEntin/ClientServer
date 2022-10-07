#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./runtests.sh 2>&1 50 | tee testslog.txt"
    exit 0
fi 

if [ $# -ne 1 ]
then
echo usage: './runtests.sh <number repeat>'
exit
pwd
fi

trap SIGHUP SIGINT SIGTERM

make -j4 testbin
./testbin --gtest_repeat=$1 --gtest_break_on_failure
