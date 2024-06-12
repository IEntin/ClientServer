#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

NUMREPEAT=500

set -e

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -gt 1 ]]
then
    echo "Usage: './runShortSessions.sh' in any one or all client directories. 
It is a test of server stability under concurrent load from multiple clients 
of different types frequently creating and destroying sessions. It can be run 
on binaries built with thread or address sanitizer or on a plain build."

     exit 0
fi

function cleanup {
    printf "\nrestoring ClientOptions.json 'RunLoop : true'\n"
    (sed -i 's/"RunLoop" : false/"RunLoop" : true/' ClientOptions.json)
    sync
}

function interrupted {
    NUMREPEAT=0
    printf "interrupted"
    sync
}

trap interrupted SIGINT

trap interrupted SIGTERM

trap cleanup EXIT

(sed -i 's/"RunLoop" : true/"RunLoop" : false/' ClientOptions.json)

for (( c=1; c<=$NUMREPEAT; c++ ))
do
    if ! pgrep -x "serverX" > /dev/null
    then
	break;
    fi
    (./clientX > /dev/null)
    sync
    printf "\nrepeated %d times \n" $c
    sync
done

date

sync
