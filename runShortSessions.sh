#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

set -e

trap "exit" SIGHUP SIGINT SIGTERM

(sed -i 's/"RunLoop" : true/"RunLoop" : false/' ClientOptions.json)

for (( c=1; c<501; c++ ))
do
    (./client)
    echo repeated $c times
done

(sed -i 's/"RunLoop" : false/"RunLoop" : true/' ClientOptions.json)
