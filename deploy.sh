#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "cd to the project root ClientServer and run this script"
    echo "./deploy.sh 2>&1 | tee deploy.txt"
    echo "Start the server in the project root terminal ./server"
    echo "and each client in created terminals ./client"
    exit 0
fi 

set -e

# create client directories

# remove existing Client* directories to start from scratch

for (( c=1; c<=5; c++ ))do
rm -rf ../Client$c
done

# create FIFO directory one level above project root
mkdir -p ../Fifos

# create client directories one level above project root
for (( c=1; c<=5; c++ ))do
mkdir -p ../Client$c
done

# create links to data directory in every client directory
# copy scripts and ClientOptions.json

for (( c=1; c<=5; c++ ))do
(cd ../Client$c; ln -sf ../ClientServer/data data; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)
done

# now all client directories have the same ClientOptions.json directing to start TCP client
# if you want to make some of the clients FIFO

for c in 2 4
do
  (cd ../Client$c;sed -i 's/"CommunicationType" : "TCP"/"CommunicationType" : "FIFO"/' ClientOptions.json)
done

# build binaries and copy client binary to ClientX directories
./compile.sh

# start client terminals

for d in 1 2 3 4 5
do
gnome-terminal -e 'sh -c "cd ../Client'$d'; exec bash"'
done
