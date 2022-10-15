#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./checkmulticlients.sh 2>&1 | tee checkmclog.txt"
    exit 0
fi 

trap "exit" SIGHUP SIGINT SIGTERM

set -e

date

SERVER_DIR=$PWD
echo $SERVER_DIR

# Start server.

./server&
SERVER_PID=$!

sleep 1

# Start clients.

/bin/cp -f client ../PrjClient2
/bin/cp -f client ../PrjClient3
/bin/cp -f client ../PrjClient4
/bin/cp -f client ../PrjClient5

sleep 1

./client > /dev/null &
$SERVER_DIR/../PrjClient2/client > /dev/null &
$SERVER_DIR/../PrjClient3/client > /dev/null &
$SERVER_DIR/../PrjClient4/client > /dev/null &
$SERVER_DIR/../PrjClient5/client > /dev/null &

sleep 100

kill -SIGINT $SERVER_PID

sleep 1

date
