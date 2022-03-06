#!/bin/bash

trap "exit" SIGHUP SIGINT SIGTERM

export CLIENT_DIRECTORY=$PWD
echo $CLIENT_DIRECTORY

$CLIENT_DIRECTORY/client > /dev/null
