#!/bin/bash

#usage:
# './profile.sh' in the server directory

trap "exit" SIGHUP SIGINT SIGTERM

date

make clean
SERVER_DIRECTORY=$PWD
echo $SERVER_DIRECTORY

COMMUNICATION_TYPE=$(grep "CommunicationType" $SERVER_DIRECTORY/client_bin/ProgramOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    FIRST_CLIENT_PROFILE=profile_client_tcp.txt
else
    FIRST_CLIENT_PROFILE=profile_client_fifo.txt
fi
COMMUNICATION_TYPE=$(grep "CommunicationType" $HOME/FIFOClient1/ProgramOptions.json)
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
$SERVER_DIRECTORY/server&
SERVER_PID=$!
echo $SERVER_PID
/bin/cp -f $SERVER_DIRECTORY/client_bin/client $HOME/FIFOClient1
sleep 2

# Start tcp or fifo client.
( cd $SERVER_DIRECTORY/client_bin; $SERVER_DIRECTORY/client_bin/profile_client.sh& )
# Start another fifo or tcp client to have a mix in server profile
# The directory $HOME/FIFOClient1 (you can change it) must exist and have a copy of
# profile_client.sh, ProgramOptions.json with appropriate settings,
# and links to source files in it.
( cd $HOME/FIFOClient1; $HOME/FIFOClient1/profile_client.sh& )
date

sleep 180
kill -SIGINT $SERVER_PID
sleep 2

gprof -b ./server gmon.out > profile_server.txt
gprof -b $SERVER_DIRECTORY/client_bin/client \
  $SERVER_DIRECTORY/client_bin/gmon.out > $SERVER_DIRECTORY/client_bin/$FIRST_CLIENT_PROFILE
gprof -b $HOME/FIFOClient1/client $HOME/FIFOClient1/gmon.out > \
  $SERVER_DIRECTORY/client_bin/$SECOND_CLIENT_PROFILE
# This directory is not under git and gmon.out must be removed 'manually'
# in order not to distort the results of the next run.
rm -f $HOME/FIFOClient1/gmon.out
date
