#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

#usage:
# './profile.sh' in the server directory

set -e

trap "exit" SIGHUP SIGINT SIGTERM

date

make cleanall
SERVER_DIR=$PWD
echo $SERVER_DIR

COMMUNICATION_TYPE=$(grep "CommunicationType" $SERVER_DIR/../Client2/ClientOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    FIRST_CLIENT_PROFILE=profile_client_tcp.txt
else
    FIRST_CLIENT_PROFILE=profile_client_fifo.txt
fi
COMMUNICATION_TYPE=$(grep "CommunicationType" $SERVER_DIR/../Client3/ClientOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    SECOND_CLIENT_PROFILE=profile_client_tcp.txt
else
    SECOND_CLIENT_PROFILE=profile_client_fifo.txt
fi

if [ "$FIRST_CLIENT_PROFILE" = "$SECOND_CLIENT_PROFILE" ]
then
    echo "Clients must have different communication types";
    exit;
fi

echo "FIRST_CLIENT_PROFILE is $FIRST_CLIENT_PROFILE"
echo "SECOND_CLIENT_PROFILE is $SECOND_CLIENT_PROFILE"

# Build profile binaries.
make -j4 PROFILE=1
# Start the server.
$SERVER_DIR/server&
SERVER_PID=$!
echo $SERVER_PID
/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../Client2
/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../Client3

# Start tcp or fifo client.
# The directory $SERVER_DIR/../Client2 must exist and have a copy of ClientOptions.json, and the link to SERVER_DIR/data directory.

cd $SERVER_DIR/../Client2
./client > /dev/null &

# Start another fifo or tcp client to have a mix in server profile
# The directory $SERVER_DIR/../Client3 must exist and have a copy of ClientOptions.json, and the link to SERVER_DIR/data directory.

cd $SERVER_DIR/../Client3
./client > /dev/null &

date

sleep 180

kill -SIGINT $SERVER_PID

cd $SERVER_DIR
gprof -b server gmon.out > profile_server.txt

cd $SERVER_DIR/../Client2
gprof -b client gmon.out > $SERVER_DIR/$FIRST_CLIENT_PROFILE

cd $SERVER_DIR/../Client3
gprof -b client gmon.out > $SERVER_DIR/$SECOND_CLIENT_PROFILE

# These directories are not under git and gmon.out must be removed 'manually'
# in order not to distort the results of the next run.
rm -f $SERVER_DIR/../Client2/gmon.out
rm -f $SERVER_DIR/../Client3/gmon.out
date
