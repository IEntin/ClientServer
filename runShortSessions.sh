#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -gt 1 ]]
then
    echo "Usage: ./runShortSessions in any or all client directories." 
    exit 0
fi

set -e

trap "exit" SIGHUP SIGINT SIGTERM

(sed -i 's/"RunLoop" : true/"RunLoop" : false/' ClientOptions.json)

for (( c=1; c<501; c++ ))
do
    (./client)
    echo repeated $c times
done

(sed -i 's/"RunLoop" : false/"RunLoop" : true/' ClientOptions.json)
