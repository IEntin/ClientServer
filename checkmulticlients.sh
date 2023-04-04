#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 || $# -gt 2 ]]
then
    echo "Usage: ./checkmulticlients.sh <number of clients> [sanitizer] 2>&1 | tee checkmclog.txt"
    exit 0
fi

export cwd

cwd=$(pwd)

echo $cwd

set -e

trap "exit" SIGHUP SIGINT SIGTERM

date

# clean Client* and Fifos directories

rm -f ../Fifos/*

export c

for (( c=1; c<=$1; c++ ))
do
    rm -f ../Client$c/*
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

# create FIFO directory and client directories at the project root level

mkdir -p ../Fifos

for (( c=1; c<=$1; c++ ))
do
    mkdir -p ../Client$c
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=$1; c++ ))
do
    (cd ../Client$c; ln -sf $cwd/data .; cp $cwd/runShortSessions.sh .; cp $cwd/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json for TCP client
# make even clients FIFO:

c=1
while [ $c -le $1 ]
do
    REMAINDER=$(( $c % 2 ))
    if [ $REMAINDER -eq 0 ]
    then
	(cd ../Client$c;sed -i 's/"CommunicationType" : "TCP"/"CommunicationType" : "FIFO"/' ClientOptions.json)
    fi
    c=$(($c+1))
done

# build binaries and copy client binary to Client* directories
#optionally you can pass SANITIZE parameter (aul or thread) as $2

make -j4 SANITIZE=$2

for (( c=1; c<=$1; c++ ))
do
    /bin/cp -f client ../Client$c
done

# Start the server

./server&
SERVER_PID=$!

echo "server pid="$SERVER_PID

sleep 1

# Start clients.

for (( c=1; c<=$1; c++ ))
do
    ( cd ../Client$c; ./client > /dev/null& )
done

sleep 60

clients=$(ps -ef | grep -w './client' | grep -v 'grep')

fifos=$(ls ../Fifos)

echo -e "\nkilling server\n"

kill $SERVER_PID

sleep 2

sync

printf "\n$clients\n"

sync

printf "\n$fifos\n"

sync

printf "\nnumber started clients=%d\n\n" $(echo "$clients" | wc -l)

sync

date

sync

sleep 1
