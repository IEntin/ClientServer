#!/bin/bash

set -e

trap "exit" SIGHUP SIGINT SIGTERM

make -j4 SANITIZE=$1 OPTIMIZE=$2 server client

cp client ../Client1

cp client ../Client2

cp client ../Client3

cp client ../Client4

cp client ../Client5

make -j4  SANITIZE=$1 OPTIMIZE=$2
