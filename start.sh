#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

set -e

trap "exit" SIGHUP SIGINT SIGTERM

for i in {1..5}
do
  ./client > /dev/null&
done

wait
