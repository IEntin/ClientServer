#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "Usage: ./checkmulticlients.sh 2>&1 | tee checkmclog.txt"
    exit 0
fi 

set -e

trap "exit" SIGHUP SIGINT SIGTERM

date

# Start the server

./server&
SERVER_PID=$!

echo "server pid="$SERVER_PID

# Start clients.

for (( c=1; c<=5; c++ ))do
/bin/cp -f client ../Client$c
sleep 0.1
cd ../Client$c
./client > /dev/null&
cd -
done

sleep 60

kill -SIGINT $SERVER_PID

date

wait
