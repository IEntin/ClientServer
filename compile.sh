#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then
    echo "This script calls make with different parameters."
    echo "If build succeeds the clientX binary is copied to"
    echo "all client directories."
    echo "Runs tests at the end."
    echo "Usage: cd to the project root and run this script"
    echo "with any combination of arguments or without arguments"
    echo "compile.sh"
    echo "CMPLR<g++ | clang++>"
    echo "SANITIZE=<aul | thread>"
    echo "OPTIMIZE=<-O0 | -O1 | -O2 | -O3>"
    echo "GDWARF=<gdwarf-5 | gdwarf-4>"
    echo "examples:"
    echo "'./compile.sh GDWARF=-gdwarf-4 OPTIMIZE=-O0 SANITIZE=aul CMPLR=g++'"
    echo "'./compile.sh'"
    echo "Run 'make cleanall' if arguments changed since the last build."
    echo "Start the server in the project root terminal: './server'"
    echo "and each client in its terminal: './clientX' or './clientX > /dev/null'"
    exit 0
fi

set -e

trap EXIT SIGHUP SIGINT SIGTERM

mkdir -p $UP_DIR/Fifos

NUMBER_CORES=$(nproc)

make -j$NUMBER_CORES $1 $2 $3 $4

for (( c=1; c<=5; c++ ))
do
    mkdir -p $UP_DIR/Client$c
    cp clientX $UP_DIR/Client$c
done

date

wait
