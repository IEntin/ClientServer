#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

# sudo apt install valgrind
# valgrind --tool=massif
# ms_print massif.out.13253 > output

# valgrind --leak-check=full --show-leak-kinds=all

# valgrind --tool=massif ./clientX > /dev/null

# valgrind --tool=massif ./serverX

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

PRJ_DIR=$(dirname $SCRIPT_DIR)
echo "PRJ_DIR:" $PRJ_DIR

UP_DIR=$(dirname $PRJ_DIR)
echo "UP_DIR:" $UP_DIR

CLIENT_DIR=$UP_DIR/$2
echo "CLIENT_DIR:" $CLIENT_DIR

pkill serverX

pkill clientX

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 2 || $# -gt 2 ]]
then
    echo "Usage: [path]/serverAllocations.sh <serverX or nothing> <Client1 or Client2>"
    echo "where Client1 is tcp client and Client2 is fifo client"
    echo "If the first parameter is not empty heap statistics is collected for the serverX."
    echo "If it is empty then statistics is for the client."
    echo "Examples:"
    echo "  scripts/heapStats.sh serverX Client1"
    echo "  scripts/heapStats.sh '' Client2"
    exit 0
fi

set -e

trap EXIT SIGHUP SIGINT SIGTERM

date

# Build binaries.

NUMBER_CORES=$(nproc)

( cd $PRJ_DIR; make cleanall; make -j$NUMBER_CORES GDWARF=-gdwarf-4 )

cp -f $PRJ_DIR/clientX $CLIENT_DIR

sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 100/' $CLIENT_DIR/ClientOptions.json

# Start the server.

cd $PRJ_DIR
if [ -z $1 ]
then
    $PRJ_DIR/serverX&
else
    valgrind --leak-check=full --show-leak-kinds=all $PRJ_DIR/serverX&
fi

APP_PID=$!
echo "APP_PID:" $APP_PID

sleep 5

( cd $CLIENT_DIR
if [ -z $1 ]
then
    valgrind --leak-check=full --show-leak-kinds=all $CLIENT_DIR/clientX > /dev/null
else
    $CLIENT_DIR/clientX > /dev/null
fi )

sed -i 's/"MaxNumberTasks" : 100/"MaxNumberTasks" : 0/' $CLIENT_DIR/ClientOptions.json

echo "Killing server"
kill $APP_PID

date
sleep 5

cd $PRJ_DIR
make cleanall
