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
    echo "If build succeeds the client binary is copied to"
    echo "Client directories."
    echo "Usage: cd to the project root and run this script"
    echo "./compile.sh GDWARF=<gdwarf-5 or gdwarf-4 or ''>"
    echo "SANITIZE=<aul or thread> OPTIMIZE=<-Ox or empty>"
    echo "examples:"
    echo "  ./compile.sh GDWARF=-gdwarf-4 SANITIZE=aul OPTIMIZE=-O2"
    echo "  ./compile.sh GDWARF=-gdwarf-4 '' OPTIMIZE=-O2"
    echo "  Next is used for valgrind tests:"
    echo "  ./compile.sh GDWARF=-gdwarf-4"
    echo "  ./compile.sh '' SANITIZE=aul"
    echo "  ./compile.sh '' '' OPTIMIZE=-O0"
    echo "  ./compile.sh"
    echo "Start the server in the project root terminal: ./server"
    echo "and each client in its terminal: ./client"
    exit 0
fi

set -e

trap EXIT SIGHUP SIGINT SIGTERM

make -j4 $1 $2 $3

for (( c=1; c<=5; c++ ))do
    mkdir -p $UP_DIR/Client$c
    cp client $UP_DIR/Client$c
done

date

wait
