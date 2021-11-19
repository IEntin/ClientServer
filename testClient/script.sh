#!/bin/bash

trap "exit" SIGHUP SIGINT SIGTERM

for number in {1..500}
do
./client
echo repeated $number times
done
