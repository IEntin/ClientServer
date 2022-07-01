#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

#usage:
# ./checkstuff.sh  2>&1 | tee checklog.txt

set -e

trap "exit" SIGHUP SIGINT SIGTERM

echo
echo "***** use g++ compiler *****"
echo
make cleanall
make -j4 CMPLR=g++

echo
echo "***** disable precompiled headers *****"
echo
make cleanall
make -j4 ENABLEPCH=0

sleep 2

echo
echo "***** address + ub + leak sanitizer *****"
echo
make cleanall
make -j4 SANITIZE=aul

sleep 2

echo
echo "***** thread sanitizer *****"
echo
make cleanall
make -j4 SANITIZE=thread
