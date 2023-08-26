#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 || $# -gt 2 ]]
then
    echo "Usage: ./checkmulticlients.sh <number of clients> [sanitizer] 2>&1 | tee checkmclog.txt"
    exit 0
fi

set -e

trap SIGHUP SIGINT SIGTERM

date

function printReport {
    printf "\n$clients\n"
    sync
    printf "\n$fifos\n"
    sync
    printf "\nnumber started clients=%d\n\n" $(echo "$clients" | wc -l)
    sync
    date
    sync
}

trap printReport EXIT

# clean Client* and Fifos directories

rm -f $UP_DIR/Fifos/*

export c

for (( c=1; c<=$1; c++ ))
do
    rm -f $UP_DIR/Client$c/*
done

# create FIFO directory and client directories at the project root level

mkdir -p $UP_DIR/Fifos

for (( c=1; c<=$1; c++ ))
do
    mkdir -p $UP_DIR/Client$c
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=$1; c++ ))
do
    (cd $UP_DIR/Client$c; ln -sf $SCRIPT_DIR/data .; cp $SCRIPT_DIR/runShortSessions.sh .; cp $SCRIPT_DIR/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json for TCP client
# make even clients FIFO:

c=1
while [ $c -le $1 ]
do
    REMAINDER=$(( $c % 2 ))
    if [ $REMAINDER -eq 0 ]
    then
	(cd $UP_DIR/Client$c;sed -i 's/"ClientType" : "TCP"/"ClientType" : "FIFO"/' ClientOptions.json)
    fi
    c=$(($c+1))
done

# build binaries and copy client binary to Client* directories
#optionally you can pass SANITIZE parameter (aul or thread) as $2

make -j4 SANITIZE=$2

for (( c=1; c<=$1; c++ ))
do
    /bin/cp -f client $UP_DIR/Client$c
done

# Start the server

./server&
SERVER_PID=$!

echo "server pid="$SERVER_PID

sleep 1

# Start clients.

for (( c=1; c<=$1; c++ ))
do
    cp .cryptoKey.sec $UP_DIR/Client$c
    ( cd $UP_DIR/Client$c; ./client > /dev/null& )
done

sleep 60

clients=$(ps -ef | grep -w './client' | grep -v 'grep')

fifos=$(ls ../Fifos)

echo -e "\nkilling server\n"

kill $SERVER_PID

sleep 5

rm -f .cryptoKey.sec

for (( c=1; c<=$1; c++ ))
do
    ( cd $UP_DIR/Client$c; rm -f .cryptoKey.sec )
done

sync
