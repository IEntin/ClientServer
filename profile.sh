#!/bin/bash

#usage:
# './profile.sh' in the server directory

trap "exit" SIGHUP SIGINT SIGTERM

date
make clean
# Build profile binaries.
make -j3 PROFILE=1
export SERVER_DIRECTORY=$PWD
echo $SERVER_DIRECTORY
# Start the server.
$SERVER_DIRECTORY/server&
export SERVER_PID=$!
echo $SERVER_PID
/bin/cp -f $SERVER_DIRECTORY/client_bin/client $HOME/FIFOClient1
sleep 2
# Start tcp or fifo client.
( cd $SERVER_DIRECTORY/client_bin && $SERVER_DIRECTORY/client_bin/profile_client.sh& )
# Start another fifo or tcp client to have a mix in server profile
# The directory $HOME/FIFOClient1 or such must exist and have a copy of
# profile_client.sh, ProgramOptions.json with appropriate settings,
# and links to source files in it.
( cd $HOME/FIFOClient1 && $HOME/FIFOClient1/profile_client.sh& )
date
sleep 180
kill -SIGINT $SERVER_PID
sleep 2
gprof -b ./server gmon.out > profile_server.txt
( cd $SERVER_DIRECTORY/client_bin && gprof -b $SERVER_DIRECTORY/client_bin/client $SERVER_DIRECTORY/client_bin/gmon.out > $SERVER_DIRECTORY/client_bin/profile_client_tcp.txt )
( cd $HOME/FIFOClient1 && gprof -b $HOME/FIFOClient1/client $HOME/FIFOClient1/gmon.out > $SERVER_DIRECTORY/client_bin/profile_client_fifo.txt )
# This directory is not under git and gmon.out must be removed 'manually'.
rm -f $HOME/FIFOClient1/gmon.out
date
