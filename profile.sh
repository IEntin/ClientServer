#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

#usage:
# './profile.sh' in the server directory

trap "exit" SIGHUP SIGINT SIGTERM

date

make cleanall
ROOT_DIR=$PWD
echo $ROOT_DIR

COMMUNICATION_TYPE=$(grep "CommunicationType" $ROOT_DIR/client_bin/ProgramOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    FIRST_CLIENT_PROFILE=profile_client_tcp.txt
else
    FIRST_CLIENT_PROFILE=profile_client_fifo.txt
fi
COMMUNICATION_TYPE=$(grep "CommunicationType" $HOME/PrjClient2/ProgramOptions.json)
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
$ROOT_DIR/server&
SERVER_PID=$!
echo $SERVER_PID
/bin/cp -f $ROOT_DIR/client_bin/client $HOME/PrjClient2
sleep 2

# Start tcp or fifo client.
( cd $ROOT_DIR/client_bin; $ROOT_DIR/client_bin/profile_client.sh& )
# Start another fifo or tcp client to have a mix in server profile
# The directory $HOME/PrjClient2 (you can change it) must exist and have a copy of
# profile_client.sh, ProgramOptions.json, and the link to ROOT_DIR/data directory.
( cd $HOME/PrjClient2; $HOME/PrjClient2/profile_client.sh& )
date

sleep 180
kill -SIGINT $SERVER_PID
sleep 2

gprof -b ./server gmon.out > profile_server.txt
gprof -b $ROOT_DIR/client_bin/client \
  $ROOT_DIR/client_bin/gmon.out > $ROOT_DIR/$FIRST_CLIENT_PROFILE
gprof -b $HOME/PrjClient2/client $HOME/PrjClient2/gmon.out > \
  $ROOT_DIR/$SECOND_CLIENT_PROFILE
# This directory is not under git and gmon.out must be removed 'manually'
# in order not to distort the results of the next run.
rm -f $HOME/PrjClient2/gmon.out
date
