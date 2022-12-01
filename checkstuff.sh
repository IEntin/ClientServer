#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./checkstuff.sh 2>&1 | tee checklog.txt"
    exit 0
fi 

set -e

trap "exit" SIGHUP SIGINT SIGTERM

sleep 2

function copyClient {
/bin/cp -f client ../Client1
/bin/cp -f client ../Client2
/bin/cp -f client ../Client3
/bin/cp -f client ../Client4
/bin/cp -f client ../Client5
}

echo
echo "***** use g++ compiler *****"
echo
make cleanall
make -j4 CMPLR=g++
copyClient

sleep 2

echo
echo "***** disable precompiled headers *****"
echo
make cleanall
make -j4 CMPLR=g++ ENABLEPCH=0
copyClient

sleep 2

echo
echo "***** address + ub + leak sanitizer *****"
make cleanall
make -j4 CMPLR=g++ SANITIZE=aul
copyClient

sleep 2

echo
echo "***** thread sanitizer *****"
echo
make cleanall
make -j4 CMPLR=g++ SANITIZE=thread
copyClient

sleep 2

echo
echo "***** use clang++ compiler *****"
echo
make cleanall
make -j4
copyClient

sleep 2

echo
echo "***** disable precompiled headers *****"
echo
make cleanall
make -j4 ENABLEPCH=0
copyClient

sleep 2

echo
echo "***** address + ub + leak sanitizer *****"
echo
make cleanall
make -j4 SANITIZE=aul
copyClient

sleep 2

echo
echo "***** thread sanitizer *****"
echo
make cleanall
make -j4 SANITIZE=thread
copyClient

#make cleanall
