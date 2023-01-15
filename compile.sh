#!/bin/bash

set -e

trap "exit" SIGHUP SIGINT SIGTERM

make -j4 SANITIZE=$1 OPTIMIZE=$2

for (( c=1; c<=5; c++ ))do
    mkdir -p ../Client$c
    cp client ../Client$c
done

date

wait
