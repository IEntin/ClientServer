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

/bin/cp -f client ../Client2
/bin/cp -f client ../Client3
/bin/cp -f client ../Client4
/bin/cp -f client ../Client5

cd $SERVER_DIR/../Client1; ./client > /dev/null &
cd $SERVER_DIR/../Client2; ./client > /dev/null &
cd $SERVER_DIR/../Client3; ./client > /dev/null &
cd $SERVER_DIR/../Client4; ./client > /dev/null &
cd $SERVER_DIR/../Client5; ./client > /dev/null &

sleep 120

kill -SIGINT $SERVER_PID

wait

date
