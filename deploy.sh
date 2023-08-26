#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "cd to the project root and run this script"
    echo "'./deploy.sh 2>&1 | tee deploy.txt'"
    echo "Start the server in the project root terminal './server'"
    echo "and each client in client terminals './client'"
    exit 0
fi

#assuming we are in gnome terminal
#kill all instances of xterm created in previous runs

pkill -f xterm

set -e

# clean Client* and Fifos directories

rm -f $UP_DIR/Fifos/*

export c

for (( c=1; c<=20; c++ ))
do
rm -f $UP_DIR/Client$c/*
done

# create FIFO directory and client directories at the project root level

mkdir -p $UP_DIR/Fifos

for (( c=1; c<=5; c++ ))
do
mkdir -p $UP_DIR/Client$c
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=5; c++ ))
do
(cd $UP_DIR/Client$c; ln -sf $SCRIPT_DIR/data .; cp $SCRIPT_DIR/scripts/runShortSessions.sh .; cp /$SCRIPT_DIR/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json for TCP client
# if you want to make some of the clients, e.g. 2 and 4, FIFO:

for c in 2 4
do
  (cd $UP_DIR/Client$c;sed -i 's/"ClientType" : "TCP"/"ClientType" : "FIFO"/' ClientOptions.json)
done

# build binaries and copy client binary to Client* directories

./compile.sh

# create client terminals

for d in 1 2 3 4 5
do
    (cd $UP_DIR/Client$d; xterm&)
done
