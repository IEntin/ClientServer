#!/bin/bash

set -e

trap "exit" SIGHUP SIGINT SIGTERM

make -j4 SANITIZE=$1 OPTIMIZE=$2 server client

cp client ../PrjClient2

cp client ../PrjClient3

cp client ../PrjClient4

cp client ../PrjClient5

make -j4  SANITIZE=$1 OPTIMIZE=$2
