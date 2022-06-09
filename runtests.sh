#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [ $# -ne 1 ]
then
echo usage: './runtests.sh <number repeat>'
exit
pwd
fi

trap "exit" SIGHUP SIGINT SIGTERM

make -j4 testbin
./testbin --gtest_repeat=$1 --gtest_break_on_failure
