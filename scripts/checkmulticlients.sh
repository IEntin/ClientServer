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

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 || $# -gt 2 ]]
then
    echo "Usage: [path]/checkmulticlients.sh <number of clients> [sanitizer] 2>&1 | tee checkmclog.txt"
    exit 0
fi

pkill serverX
pkill clientX

set -e

trap SIGHUP SIGINT SIGTERM

date

function printReport {
    printf "\n$clients\n"
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
    (cd $UP_DIR/Client$c; ln -sf $PRJ_DIR/data .; cp $PRJ_DIR/scripts/runShortSessions.sh .; cp $PRJ_DIR/client_src/ClientOptions.json .)
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

# build binaries and copy clientX binary to Client* directories
#optionally you can pass SANITIZE parameter (aul or thread) as $2

make -j4 SANITIZE=$2

for (( c=1; c<=$1; c++ ))
do
    /bin/cp -f clientX $UP_DIR/Client$c
done

# Start the server

$PRJ_DIR/serverX&

sleep 1

# Start clients.

for (( c=1; c<=$1; c++ ))
do
    ( cd $UP_DIR/Client$c; ./clientX > /dev/null& )
done

sleep 60

clients=$(ps -ef | grep -w './clientX' | grep -v 'grep')

echo -e "\nkilling server\n"

pkill serverX

sleep 5

sync
