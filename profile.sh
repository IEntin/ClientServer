#!/bin/bash

#usage:
# '. profile.sh' in the server directory

trap "exit" SIGHUP SIGINT SIGTERM

make clean
make -j3 PROFILE=1
export SERVER_DIRECTORY=$PWD
echo $SERVER_DIRECTORY
$SERVER_DIRECTORY/server&
export SERVER_PID=$!
echo $SERVER_PID
sleep 2
( cd $SERVER_DIRECTORY/client_bin && $SERVER_DIRECTORY/client_bin/profile_client.sh& )
sleep 180
kill -SIGINT $SERVER_PID
sleep 2
gprof -b ./server gmon.out > profile_server.txt
( cd $SERVER_DIRECTORY/client_bin && gprof -b $SERVER_DIRECTORY/client_bin/client $SERVER_DIRECTORY/client_bin/gmon.out > $SERVER_DIRECTORY/client_bin/profile_client_tcp.txt )
