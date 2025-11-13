#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

trap EXIT SIGHUP SIGINT SIGTERM

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./longtests.sh 2>&1 | tee longtests.txt"
    exit 0
fi 

set -e

$SCRIPT_DIR/scripts/checkcompile.sh
$SCRIPT_DIR/scripts/runtests.sh 10
$SCRIPT_DIR/scripts/checkmulticlients.sh 20 thread
