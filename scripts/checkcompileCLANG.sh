#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

PRJ_DIR=$(dirname $SCRIPT_DIR)
echo "PRJ_DIR:" $PRJ_DIR

UP_DIR=$(dirname $PRJ_DIR)
echo "UP_DIR:" $UP_DIR

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: [path]/checkcompileCLANG.sh 2>&1 | tee checklogCLANG.txt"
    exit 0
fi 

set -e

trap EXIT SIGHUP SIGINT SIGTERM

sleep 2

function copyClient {
for (( c=1; c<=5; c++ ))
do
    mkdir -p $UP_DIR/Client$c
    /bin/cp -f clientX $UP_DIR/Client$c
done
}

NUMBER_CORES=$(nproc)

echo
echo "***** clang++ compiler *****"

echo
make cleanall
make -j$NUMBER_CORES
copyClient

sleep 2

echo
echo "***** disable precompiled headers *****"
echo
make cleanall
make -j$NUMBER_CORES ENABLEPCH=0
copyClient

sleep 2

echo
echo "***** address + ub + leak sanitizer *****"
echo
make cleanall
make -j$NUMBER_CORES SANITIZE=aul
copyClient

sleep 2

echo
echo "***** thread sanitizer *****"
make -j$NUMBER_CORES SANITIZE=thread
copyClient
