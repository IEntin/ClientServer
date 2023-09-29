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

CLIENT_DIR=$UP_DIR/$2
echo "CLIENT_DIR:" $CLIENT_DIR

pids=$(pidof $PRJ_DIR/server)
if [ ! -z "$pids" ];
then
   kill -9 $pids
fi

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 2 || $# -gt 2 ]]
then
    echo "Usage: [path]/serverAllocations.sh <server or nothing> <Client1 or Client2>"
    echo "where Client1 is tcp client and Client2 is fifo client"
    echo "If the first parameter is not empty heap statistics is collected for the server."
    echo "If it is empty then statistics is for the client."
    echo "Examples:"
    echo "  scripts/heapStats.sh server Client1"
    echo "  scripts/heapStats.sh '' Client2"
    exit 0
fi

set -e

trap EXIT SIGHUP SIGINT SIGTERM

date

# Build binaries.
( cd $PRJ_DIR; make cleanall; make -j4 GDWARF=-gdwarf-4 )

cp -f $PRJ_DIR/client $CLIENT_DIR

sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 100/' $CLIENT_DIR/ClientOptions.json

# Start the server.

cd $PRJ_DIR
if [ -z $1 ]
then
    ./server&
else
    valgrind --leak-check=full --show-leak-kinds=all ./server&
fi

SERVER_PID=$!
echo "SERVER_PID:" $SERVER_PID

sleep 5

cp -f $PRJ_DIR/.cryptoKey.sec $CLIENT_DIR

( cd $CLIENT_DIR
if [ -z $1 ]
then
    valgrind --leak-check=full --show-leak-kinds=all ./client > /dev/null
else
    ./client > /dev/null
fi )

sed -i 's/"MaxNumberTasks" : 100/"MaxNumberTasks" : 0/' $CLIENT_DIR/ClientOptions.json

echo "Killing server"
kill $SERVER_PID

date
sleep 5
