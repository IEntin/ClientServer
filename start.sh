#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

set -e

trap "exit" SIGHUP SIGINT SIGTERM

if [[ $@ == "--help" || $@ == "-h" || $# -ne 1 ]]
then 
    echo "Usage: ./starts.sh <number of clients>"
    echo "Prerequisites:"
    echo "Set \"CommunicationType\" : \"FIFO\""
    echo "in the ClientOptions.json."
    echo "Start the server in a different shell"
    echo "or in the same shell in the background."
    echo "You can now run ./start.sh <N>."
    echo "top or ps shows server and all expected clients running,"
    echo "all clients consuming roughly the same CPU."
    echo "While the script is running you can see generated fifos"
    echo "in the \"FifoDirectoryName\" whatever it is."
    echo "You can run the same test with \"CommunicationType\" : \"TCP\""
    echo "To stop the script kill the server with Ctrl+C or other signal."

    exit
fi 

for (( c=1; c<=$1; c++ ))do
  ./client > /dev/null&
done

wait
