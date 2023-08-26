#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./checkstuff.sh 2>&1 | tee checklog.txt"
    exit 0
fi 

set -e

trap EXIT SIGHUP SIGINT SIGTERM

sleep 2

function copyClient {
/bin/cp -f client $UP_DIR/Client1
/bin/cp -f client $UP_DIR/Client2
/bin/cp -f client $UP_DIR/Client3
/bin/cp -f client $UP_DIR/Client4
/bin/cp -f client $UP_DIR/Client5
}

echo
echo "***** g++ compiler *****"

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
echo
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

echo
echo "***** clang++ compiler *****"

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
