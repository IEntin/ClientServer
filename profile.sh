#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

#usage:
# './profile.sh' in the server directory

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

set -e

trap EXIT SIGHUP SIGINT SIGTERM

date

make cleanall

COMMUNICATION_TYPE=$(grep "ClientType" $UP_DIR/Client2/ClientOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    FIRST_CLIENT_PROFILE=$SCRIPT_DIR/profiles/profile_client_tcp.txt
else
    FIRST_CLIENT_PROFILE=$SCRIPT_DIR/profiles/profile_client_fifo.txt
fi
COMMUNICATION_TYPE=$(grep "ClientType" $UP_DIR/Client3/ClientOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    SECOND_CLIENT_PROFILE=$SCRIPT_DIR/profiles/profile_client_tcp.txt
else
    SECOND_CLIENT_PROFILE=$SCRIPT_DIR/profiles/profile_client_fifo.txt
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
$SCRIPT_DIR/server&
SERVER_PID=$!
echo $SERVER_PID
/bin/cp -f $SCRIPT_DIR/client $SCRIPT_DIR/.cryptoKey.sec $UP_DIR/Client2

/bin/cp -f $SCRIPT_DIR/client $SCRIPT_DIR/.cryptoKey.sec $UP_DIR/Client3

# Start tcp or fifo client.
# The directory Client2 must exist and have a copy of ClientOptions.json, and the link to SCRIPT_DIR/data directory.

cd $UP_DIR/Client2
./client > /dev/null &

# Start another fifo or tcp client to have a mix in server profile
# The directory Client3 must exist and have a copy of ClientOptions.json, and the link to SCRIPT_DIR/data directory.

cd $UP_DIR/Client3
./client > /dev/null &

date

sleep 60

kill -SIGINT $SERVER_PID

cd $SCRIPT_DIR
gprof -b server gmon.out > profiles/profile_server.txt

cd $UP_DIR/Client2
gprof -b client gmon.out > $FIRST_CLIENT_PROFILE

cd $UP_DIR/Client3
gprof -b client gmon.out > $SECOND_CLIENT_PROFILE

# These directories are not under git and gmon.out must be removed 'manually'
# in order not to distort the results of the next run.

(cd $SCRIPT_DIR; rm -f gmon.out .cryptoKey.sec)
(cd $UP_DIR/Client2; rm -f gmon.out .cryptoKey.sec)
(cd $UP_DIR/Client3; rm -f gmon.out .cryptoKey.sec)
date
