#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then 
    echo "cd to the project root ClientServer and run this script"
    echo "./deploy.sh 2>&1 | tee deploy.txt"
    echo "start the server in project root ./server"
    echo "open terminal for every client and start the client ./client"
    exit 0
fi 

# create client directories

# optionally remove existing Client* directories to start from scratch
#rm -rf ../Client1 ../Client2 ../Client3 ../Client4 ../Client5

# create FIFO directory one level above project root
mkdir -p ../Fifos

# create client directories one level above project root
mkdir -p ../Client1
mkdir -p ../Client2
mkdir -p ../Client3
mkdir -p ../Client4
mkdir -p ../Client5

# create links to data directory in every client directory
# copy scripts and ClientOptions.json
(cd ../Client1; ln -sf ../ClientServer/data data; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)
(cd ../Client2; ln -sf ../ClientServer/data data; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)
(cd ../Client3; ln -sf ../ClientServer/data data; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)
(cd ../Client4; ln -sf ../ClientServer/data data; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)
(cd ../Client5; ln -sf ../ClientServer/data data; cp ../ClientServer/script.sh .; cp ../ClientServer/ClientOptions.json .)

# now all client directories have the same ClientOptions.json directing to start TCP client
# if you want to make some of the clients FIFO
(cd ../Client2;sed -i 's/"CommunicationType" : "TCP"/"CommunicationType" : "FIFO"/' ClientOptions.json)
(cd ../Client4;sed -i 's/"CommunicationType" : "TCP"/"CommunicationType" : "FIFO"/' ClientOptions.json)

# build binaries and copy client binary to ClientX directories
./compile.sh
