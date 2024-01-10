#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

#usage:
# './profile.sh' in the server directory

SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
echo "SCRIPT_DIR:" $SCRIPT_DIR

PRJ_DIR=$SCRIPT_DIR
echo "PRJ_DIR:" $PRJ_DIR

UP_DIR=$(dirname $SCRIPT_DIR)
echo "UP_DIR:" $UP_DIR

CLIENT_DIR2=$UP_DIR/Client2
echo "CLIENT_DIR2:" $CLIENT_DIR2

CLIENT_DIR3=$UP_DIR/Client3
echo "CLIENT_DIR3:" $CLIENT_DIR3

pkill server

set -e

trap EXIT SIGHUP SIGINT SIGTERM

date

make cleanall

COMMUNICATION_TYPE=$(grep "ClientType" $UP_DIR/Client2/ClientOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    FIRST_CLIENT_PROFILE=$PRJ_DIR/profiles/profile_client_tcp.txt
else
    FIRST_CLIENT_PROFILE=$PRJ_DIR/profiles/profile_client_fifo.txt
fi
COMMUNICATION_TYPE=$(grep "ClientType" $UP_DIR/Client3/ClientOptions.json)
if [[ "$COMMUNICATION_TYPE" == *"TCP"* ]]
then
    SECOND_CLIENT_PROFILE=$PRJ_DIR/profiles/profile_client_tcp.txt
else
    SECOND_CLIENT_PROFILE=$PRJ_DIR/profiles/profile_client_fifo.txt
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

$PRJ_DIR/server&

sleep 2

cp -f $PRJ_DIR/client $PRJ_DIR/.cryptoKey.sec $CLIENT_DIR2

cp -f $PRJ_DIR/client $PRJ_DIR/.cryptoKey.sec $CLIENT_DIR3

date

# Start tcp or fifo client.
# The directory Client2 must exist and have a copy of ClientOptions.json, and the link to SCRIPT_DIR/data directory.

( cd $CLIENT_DIR2
sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 1000/' $CLIENT_DIR2/ClientOptions.json
./client > /dev/null )

# Start another fifo or tcp client to have a mix in server profile
# The directory Client3 must exist and have a copy of ClientOptions.json, and the link to SCRIPT_DIR/data directory.

( cd $UP_DIR/Client3
sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 1000/' $CLIENT_DIR3/ClientOptions.json
./client > /dev/null )

sed -i 's/"MaxNumberTasks" : 1000/"MaxNumberTasks" : 0/' $CLIENT_DIR2/ClientOptions.json
sed -i 's/"MaxNumberTasks" : 1000/"MaxNumberTasks" : 0/' $CLIENT_DIR3/ClientOptions.json

date

pkill server

cd $SCRIPT_DIR
gprof -b server gmon.out > profiles/profile_server.txt

( cd $CLIENT_DIR2; gprof -b client gmon.out > $FIRST_CLIENT_PROFILE )

( cd $CLIENT_DIR3; gprof -b client gmon.out > $SECOND_CLIENT_PROFILE )

# These directories are not under git and gmon.out must be removed 'manually'
# in order not to distort the results of the next run.

( cd $SCRIPT_DIR; rm -f gmon.out .cryptoKey.sec )
( cd $CLIENT_DIR2; rm -f gmon.out .cryptoKey.sec )
( cd $CLIENT_DIR3; rm -f gmon.out .cryptoKey.sec )
date
