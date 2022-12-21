#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -ne 1 ]]
then 
    echo "Usage: ./checkmulticlients.sh 2>&1 <number of clients of every type> | tee checkmclog.txt"
    exit 0
fi 

set -e

trap "exit" SIGHUP SIGINT SIGTERM

date

# remove Fifos and Client* directories to start from scratch

export c

rm -rf ../Fifos

for (( c=1; c<=2*$1; c++ ))
do
    rm -rf ../Client? ../Client??
done

# create FIFO directory and client directories at the project root level

mkdir -p ../Fifos

for (( c=1; c<=2*$1; c++ ))
do
    mkdir -p ../Client$c
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=2*$1; c++ ))
do
    (cd ../Client$c; ln -sf ../ClientServer/data .; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json directing to start TCP client
# make second half of the clients FIFO:

for (( c=$1+1; c<=2*$1; c++ ))
do
    (cd ../Client$c;sed -i 's/"CommunicationType" : "TCP"/"CommunicationType" : "FIFO"/' ClientOptions.json)
done

# build binaries and copy client binary to Client* directories
# you can pass SANITIZE parameter (aul || thread) as $2

make -j4 SANITIZE=$2

for (( c=1; c<=2*$1; c++ ))
do
    /bin/cp -f client ../Client$c
done

# Start the server

./server&
SERVER_PID=$!

echo "server pid="$SERVER_PID

sleep 1

# Start clients.

for (( c=1; c<=2*$1; c++ ))
do
    ( cd ../Client$c; ./client > /dev/null& )
done

sleep 100

kill -SIGINT $SERVER_PID

sleep 1

date
