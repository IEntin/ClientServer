#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=~/ClientServer/scripts
echo "SCRIPT_DIR:" $SCRIPT_DIR

PRJ_DIR=$(dirname $SCRIPT_DIR)
echo "PRJ_DIR:" $PRJ_DIR

UP_DIR=$(dirname $PRJ_DIR)
echo "UP_DIR:" $UP_DIR

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 ]]
then
    echo "Usage: scripts/valgrindTest.sh <tcp or fifo>"
    exit
fi

pkill serverX

pkill clientX

set -e

trap SIGHUP SIGINT SIGTERM

date

# clean Client* and Fifos directories

rm -f $UP_DIR/Fifos/*

rm -rf $UP_DIR/Client

# create FIFO and client directores at the project root level

mkdir -p $UP_DIR/Fifos

mkdir -p $UP_DIR/Client

# create data directory links in every client directory
# copy scripts and ClientOptions.json

(cd $UP_DIR/Client; ln -sf $PRJ_DIR/data .; cp $PRJ_DIR/client_src/ClientOptions.json .)

# build binaries and copy clientX binary to Client directory

make cleanall
./compile.sh
/bin/cp -f clientX $UP_DIR/Client

# Start the server

$PRJ_DIR/serverX&

sleep 1

# Start the client

( cd $UP_DIR/Client;
if [[ $1 == fifo ]]; then
    sed -i 's/"ClientType" : "TCP"/"ClientType" : "FIFO"/' ClientOptions.json
elif [[ $1 == tcp ]]; then
    sed -i 's/"ClientType" : "FIFO"/"ClientType" : "TCP"/' ClientOptions.json
fi )

( cd $UP_DIR/Client;
  sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 100/' ClientOptions.json
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $PRJ_DIR/clientX > /dev/null
  sed -i 's/"MaxNumberTasks" : 100/"MaxNumberTasks" : 0/' ClientOptions.json
)

echo -e "\nkilling server\n"

pkill serverX

wait
