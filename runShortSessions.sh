#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

set -e

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -gt 1 ]]
then
    echo "Usage: './runShortSessions.sh' in any one or any number of 
or all client directories. It is a test of server stability under 
concurrent load from multiple clients of different types frequently 
creating and destroying sessions. It can be ran on binaries built 
with thread or address sanitizer or on a plain build."

     exit 0
fi

function cleanup {
    echo "restoring ClientOptions.json RunLoop : true"
    (sed -i 's/"RunLoop" : false/"RunLoop" : true/' ClientOptions.json)
}

function interrupted {
    echo "interrupted"
}

trap interrupted SIGINT

trap interrupted SIGTERM

trap cleanup EXIT

(sed -i 's/"RunLoop" : true/"RunLoop" : false/' ClientOptions.json)

for (( c=1; c<501; c++ ))
do
    (./client)
    echo repeated $c times
done

sync
date
sync
