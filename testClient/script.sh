#!/bin/bash

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
