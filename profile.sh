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

pkill serverX

pkill clientX

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

NUMBER_CORES=$(nproc)

make -j$NUMBER_CORES PROFILE=1

# Start the server.

$PRJ_DIR/serverX&

sleep 2

cp -f $PRJ_DIR/clientX $CLIENT_DIR2

cp -f $PRJ_DIR/clientX $CLIENT_DIR3

date

# Start tcp or fifo client.
# The directory Client2 must exist and have a copy of ClientOptions.json, and the link to SCRIPT_DIR/data directory.

cd $CLIENT_DIR2
sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 1000/' $CLIENT_DIR2/ClientOptions.json
./clientX > /dev/null&

cd $UP_DIR/Client3
sed -i 's/"MaxNumberTasks" : 0/"MaxNumberTasks" : 1000/' $CLIENT_DIR3/ClientOptions.json
./clientX > /dev/null

sleep 2

sed -i 's/"MaxNumberTasks" : 1000/"MaxNumberTasks" : 0/' $CLIENT_DIR2/ClientOptions.json
sed -i 's/"MaxNumberTasks" : 1000/"MaxNumberTasks" : 0/' $CLIENT_DIR3/ClientOptions.json

date

pkill serverX

sleep 2

cd $SCRIPT_DIR
gprof -b serverX gmon.out > profiles/profile_server.txt

( cd $CLIENT_DIR2; gprof -b clientX gmon.out > $FIRST_CLIENT_PROFILE )

( cd $CLIENT_DIR3; gprof -b clientX gmon.out > $SECOND_CLIENT_PROFILE )

# These directories are not under git and gmon.out must be removed 'manually'
# in order not to distort the results of the next run.

( cd $SCRIPT_DIR; rm -f gmon.out )
( cd $CLIENT_DIR2; rm -f gmon.out )
( cd $CLIENT_DIR3; rm -f gmon.out )

cd $PRJ_DIR
make cleanall

date
