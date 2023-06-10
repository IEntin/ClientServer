#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

tar xzvf $1

dirname=$(basename "$1" '.tar.gz')

(cd $dirname; make)

echo -e "\n$dirname\n"
