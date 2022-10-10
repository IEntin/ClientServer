#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./checkmulticlients.sh 2>&1 | tee checkmclog.txt"
    exit 0
fi 

trap SIGHUP SIGINT SIGTERM

set -e

date

SERVER_DIR=$PWD
echo $SERVER_DIR

# Start server.

$SERVER_DIR/server&
SERVER_PID=$!

sleep 1

# Start clients.

/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../PrjClient2
/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../PrjClient3
/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../PrjClient4
/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../PrjClient5

sleep 1

$SERVER_DIR/client > /dev/null &
cd $SERVER_DIR/../PrjClient2; ./client > /dev/null &
cd $SERVER_DIR/../PrjClient3; ./client > /dev/null &
cd $SERVER_DIR/../PrjClient4; ./client > /dev/null &
cd $SERVER_DIR/../PrjClient5; ./client > /dev/null &

sleep 100

kill -SIGINT $SERVER_PID

sleep 1

date
