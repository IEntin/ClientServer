#!/bin/bash

# To use this script disable infinite loop in ProgramOptions.json
# for the client: set "RunLoop" : false,

trap "exit" SIGHUP SIGINT SIGTERM

for number in {1..500}
do
./client
if [ $? -ne 0 ]
then 
  exit 1 
fi
echo repeated $number times
done
