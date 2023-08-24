#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

trap EXIT SIGHUP SIGINT SIGTERM

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./longtests.sh 2>&1 | tee longtests.txt"
    exit 0
fi 

set -e

scripts/checkcompile.sh
scripts/runtests.sh 50
scripts/checkmulticlients.sh 20 thread
