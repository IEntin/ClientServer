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

COMMUNICATION_TYPE=$(grep "CommunicationType" $SERVER_DIR/../PrjClient2/ClientOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    FIRST_CLIENT_PROFILE=profile_client_tcp.txt
else
    FIRST_CLIENT_PROFILE=profile_client_fifo.txt
fi
COMMUNICATION_TYPE=$(grep "CommunicationType" $SERVER_DIR/../PrjClient3/ClientOptions.json)
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
/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../PrjClient2
/bin/cp -f $SERVER_DIR/client $SERVER_DIR/../PrjClient3
sleep 2

# Start tcp or fifo client.
# The directory $SERVER_DIR/../PrjClient2 (you can change it) must exist and have a copy of
# profile_client.sh, ClientOptions.json, and the link to SERVER_DIR/data directory.
( cd $SERVER_DIR/../PrjClient2; $SERVER_DIR/../PrjClient2/profile_client.sh& )
# Start another fifo or tcp client to have a mix in server profile
# The directory $SERVER_DIR/../PrjClient3 (you can change it) must exist and have a copy of
# profile_client.sh, ClientOptions.json, and the link to SERVER_DIR/data directory.
( cd $SERVER_DIR/../PrjClient3; $SERVER_DIR/../PrjClient3/profile_client.sh& )
date

sleep 60
kill -SIGINT $SERVER_PID
sleep 2

gprof -b ./server gmon.out > profile_server.txt
gprof -b $SERVER_DIR/../PrjClient2/client \
  $SERVER_DIR/../PrjClient2/gmon.out > $SERVER_DIR/$FIRST_CLIENT_PROFILE
gprof -b $SERVER_DIR/../PrjClient3/client \
  $SERVER_DIR/../PrjClient3/gmon.out > $SERVER_DIR/$SECOND_CLIENT_PROFILE
# These directories are not under git and gmon.out must be removed 'manually'
# in order not to distort the results of the next run.
rm -f $SERVER_DIR/../PrjClient2/gmon.out
rm -f $SERVER_DIR/../PrjClient3/gmon.out
date
