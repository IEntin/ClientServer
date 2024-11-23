#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then
    echo "Usage: sudo scripts/installLZ4.sh"
    exit 0
fi
cd /usr/local
git clone https://github.com/lz4/lz4.git
cd lz4
make
make install
