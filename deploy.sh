#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

if [[ ( $@ == "--help") ||  $@ == "-h"  || $# -lt 1 || $# -gt 2 ]]
then 
    echo "cd to the project root and run this script"
    echo "'./deploy.sh <number of client terminals>'"
    echo "If app was not built previously it will be built"
    echo "here with default arguments."
    echo "Script creates $1 clients to fit into display size."
    echo "of intermitten types, TCP or FIFO."
    echo "Start the server in the project root terminal './serverX'"
    echo "and each client in its terminal"
    echo "'./clientX' or './clientX > /dev/null'"
    exit 0
fi

#Kill all instances of xterm created in previous runs

pkill -f xterm

set -e

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

# build binaries if needed

cd $SCRIPT_DIR
./compile.sh

for ((c=1; c<=$1; c++))
do
    cp $SCRIPT_DIR/clientX $UP_DIR/Client$c
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=$1; c++ ))
do
    (cd $UP_DIR/Client$c;\
     ln -sf $SCRIPT_DIR/data .;\
     cp $SCRIPT_DIR/scripts/runShortSessions.sh .;\
     cp /$SCRIPT_DIR/client_src/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json for TCP client
# if you want to make some of the clients, e.g. 2, 4, 6 ... FIFO:

for (( c=1; c<=$1; c++ ))
do
    if [[ $(($c%2)) -eq 0 ]]
    then
       (cd $UP_DIR/Client$c;sed -i 's/"ClientType" : "TCP"/"ClientType" : "FIFO"/' ClientOptions.json)
    fi
done

# create client terminals

for (( c=1; c<=$1; c++ ))
do
    (cd $UP_DIR/Client$c; xterm -geometry 67x24 -bg white -fg black&)
done
