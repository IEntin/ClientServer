#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

trap "exit" SIGHUP SIGINT SIGTERM

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./longtests.sh 2>&1 | tee longtests.txt"
    exit 0
fi 

set -e

./checkstuff.sh
./runtests.sh
./checkmulticlients.sh
