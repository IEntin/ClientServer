#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

set -e

trap EXIT SIGHUP SIGINT SIGTERM

make -j4 SANITIZE=$1 OPTIMIZE=$2

for (( c=1; c<=5; c++ ))do
    mkdir -p $UP_DIR/Client$c
    cp client $UP_DIR/Client$c
done

date

wait
