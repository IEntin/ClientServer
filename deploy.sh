#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "cd to the project root and run this script"
    echo "'./deploy.sh 2>&1 | tee deploy.txt'"
    echo "Start the server in the project root terminal './server'"
    echo "and each client in client terminals './client'"
    exit 0
fi

export cwd

cwd=$(pwd)

echo $cwd

#assuming we are in gnome terminal
#kill all instances of xterm created in previous runs

pkill -f xterm

set -e

# clean Client* and Fifos directories

rm -f ../Fifos/*

export c

for (( c=1; c<=20; c++ ))
do
rm -f ../Client$c/*
done

# create FIFO directory and client directories at the project root level

mkdir -p ../Fifos

for (( c=1; c<=5; c++ ))
do
mkdir -p ../Client$c
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=5; c++ ))
do
(cd ../Client$c; ln -sf $cwd/data .; cp $cwd/script.sh .; cp /$cwd/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json for TCP client
# if you want to make some of the clients, e.g. 2 and 4, FIFO:

for c in 2 4
do
  (cd ../Client$c;sed -i 's/"CommunicationType" : "TCP"/"CommunicationType" : "FIFO"/' ClientOptions.json)
done

# build binaries and copy client binary to Client* directories

./compile.sh

# create client terminals

for d in 1 2 3 4 5
do
    (cd ../Client$d; xterm&)
done
