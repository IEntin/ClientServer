#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Enable encryption in test clients."
    echo "cd to the project root and run this script"
    echo "after server startup."
    echo "Test client directories are numbered."
    echo "With 20 client directories this command"
    echo "will copy crypto files to Client1 ... Client20:"
    echo "./copyCryptoFiles.sh <number of clients>"
    exit 0
fi

for (( c=1; c<=$1; c++ ))
do
    cp .cryptoKey.sec .cryptoIv.sec ../Client$c
done

set -e

trap SIGHUP SIGINT SIGTERM

date
