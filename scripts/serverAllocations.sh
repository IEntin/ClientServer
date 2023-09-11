#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

#usage:
# './profile.sh' in the server directory

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

PRJ_DIR=$(dirname $SCRIPT_DIR)
echo "PRJ_DIR:" $PRJ_DIR

UP_DIR=$(dirname $PRJ_DIR)
echo "UP_DIR:" $UP_DIR

CLIENT_DIR=$UP_DIR/Client1
echo "CLIENT_DIR:" $CLIENT_DIR

set -e

trap EXIT SIGHUP SIGINT SIGTERM

date

# Build binaries.
( cd $PRJ_DIR; make cleanall; make -j4 GDWARF=-gdwarf-4 )

# Start the server.
cd $PRJ_DIR
valgrind $PRJ_DIR/server&
SERVER_PID=$!
echo "SERVER_PID:" $SERVER_PID

sleep 5

( cd $CLIENT_DIR; /bin/cp -f $PRJ_DIR/client $PRJ_DIR/.cryptoKey.sec .;
  sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 100/' ClientOptions.json;
  ./client > /dev/null;
  sed -i 's/"MaxNumberTasks" : 100/"MaxNumberTasks" : 0/' ClientOptions.json);

#sleep 60

echo "Killing server"
kill $SERVER_PID

date
sleep 5
