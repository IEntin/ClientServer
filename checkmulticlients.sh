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

date

SERVER_DIR=$PWD

echo $SERVER_DIR

# Start the server if not running

SERVER_PID=$(pidof server)

if [[ -z $SERVER_PID ]]
then
    ./server&
fi

SERVER_PID=$(pidof server)

echo $SERVER_PID

set -e

# Start clients.

/bin/cp -f client ../PrjClient2
/bin/cp -f client ../PrjClient3
/bin/cp -f client ../PrjClient4
/bin/cp -f client ../PrjClient5

./client > /dev/null &
cd $SERVER_DIR/../PrjClient2; ./client > /dev/null &
cd $SERVER_DIR/../PrjClient3; ./client > /dev/null &
cd $SERVER_DIR/../PrjClient4; ./client > /dev/null &
cd $SERVER_DIR/../PrjClient5; ./client > /dev/null &

sleep 120

kill -SIGINT $SERVER_PID

wait

date
