#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

# To use this script disable infinite loop in ProgramOptions.json
# for the client: set "RunLoop" : false,

set -e

trap "exit" SIGHUP SIGINT SIGTERM

for number in {1..500}
do
./client
echo repeated $number times
done
