#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

trap "exit" SIGHUP SIGINT SIGTERM

for i in 1 2 3 4 5
do
  ./client > /dev/null&
done
