#!/bin/bash

#usage:
# './profile.sh' in the server directory

trap "exit" SIGHUP SIGINT SIGTERM

date
make clean
make -j3 PROFILE=1
export SERVER_DIRECTORY=$PWD
echo $SERVER_DIRECTORY
$SERVER_DIRECTORY/server&
export SERVER_PID=$!
echo $SERVER_PID
(cd $SERVER_DIRECTORY/client_bin; \cp $SERVER_DIRECTORY/client_bin/client $HOME/FIFOClient1 )
sleep 2
# start tcp or fifo client
( cd $SERVER_DIRECTORY/client_bin && $SERVER_DIRECTORY/client_bin/profile_client.sh& )
# start fifo or tcp client to have a mix in server profile
( cd $HOME/FIFOClient1 && $HOME/FIFOClient1/profile_client.sh& )
date
sleep 180
kill -SIGINT $SERVER_PID
sleep 2
gprof -b ./server gmon.out > profile_server.txt
( cd $SERVER_DIRECTORY/client_bin && gprof -b $SERVER_DIRECTORY/client_bin/client $SERVER_DIRECTORY/client_bin/gmon.out > $SERVER_DIRECTORY/client_bin/profile_client_tcp.txt )
( cd $HOME/FIFOClient1 && gprof -b $HOME/FIFOClient1/client $HOME/FIFOClient1/gmon.out > $HOME/FIFOClient1/profile_client_fifo.txt; mv $HOME/FIFOClient1/profile_client_fifo.txt $SERVER_DIRECTORY/client_bin; rm -f $HOME/FIFOClient1/gmon.out )

date
