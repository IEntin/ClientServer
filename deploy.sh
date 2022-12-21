#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "cd to the project root ClientServer and run this script"
    echo "'./deploy.sh 2>&1 | tee deploy.txt'"
    echo "Start the server in the project root terminal './server'"
    echo "and each client in client terminals './client'"
    echo "Warning: if you repeatedly run this script close"
    echo "previously opened client terminals."
    exit 0
fi 

set -e

# create client directories

# remove Client* and Fifos directories to start from scratch

rm -rf ../Fifos

export c

for (( c=1; c<=20; c++ ))do
rm -rf ../Client$c
done

# create FIFO directory and client directories at the project root level

mkdir -p ../Fifos

for (( c=1; c<=5; c++ ))do
mkdir -p ../Client$c
done

# create data directory links in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=5; c++ ))do
(cd ../Client$c; ln -sf ../ClientServer/data .; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json directing to start TCP client
# if you want to make some of the clients, e.g. 2 and 4, FIFO:

for c in 2 4
do
  (cd ../Client$c;sed -i 's/"CommunicationType" : "TCP"/"CommunicationType" : "FIFO"/' ClientOptions.json)
done

# build binaries and copy client binary to Client* directories

./compile.sh

# start client terminals

for d in 1 2 3 4 5
do
    (cd ../Client$d; gnome-terminal&)
done
