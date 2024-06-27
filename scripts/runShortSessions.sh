#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

set -e

if [[ $@ == "--help" ||  $@ == "-h" || $# -ne 1 ]]
then
    echo "Usage: scripts/runShortSessions.sh <times to repeat>"
    echo "in any or many client directories"
    echo "It is a test of server stability under concurrent load from multiple clients"
    echo "of different types frequently creating and destroying sessions. It can be run"
    echo "on binaries built with thread or address sanitizer or on a plain build."
    exit 0
fi

NUMREPEAT=$1

function cleanup {
    printf "\nrestoring ClientOptions.json 'RunLoop : true'\n"
    sed -i 's/"RunLoop" : false/"RunLoop" : true/' ClientOptions.json
}

function interrupted {
    NUMREPEAT=0
    printf "interrupted"
}

trap interrupted SIGINT

trap interrupted SIGTERM

trap cleanup EXIT

sed -i 's/"RunLoop" : true/"RunLoop" : false/' ClientOptions.json

for (( c=1; c<=NUMREPEAT; c++ ))
do
    if ! pgrep serverX
    then
	break;
    fi
    ./clientX > /dev/null
    wait $!
    printf "\nrepeated %d times \n" $c
done

date
