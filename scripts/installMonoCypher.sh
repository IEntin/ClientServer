#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help" ||  $@ == "-h")]]
then
    echo "Usage: sudo scripts/installMonoCtpher"
    exit 0
fi

cp data/monocypher-4.0.3.tar.gz /usr/local
cd /usr/local
tar -xzvf monocypher-4.0.3.tar.gz
TRUE_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)
cp monocypher-4.0.3/src/monocypher.h "$TRUE_HOME/ClientServer/common"
cp monocypher-4.0.3/src/monocypher.c "$TRUE_HOME/ClientServer/common/monocypher.cpp"
rm monocypher-4.0.3.tar.gz
