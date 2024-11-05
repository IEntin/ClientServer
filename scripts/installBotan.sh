#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 || $# -gt 2 ]]
then
    echo "Usage: sudo scripts/installBotan.sh <botan release>"
    echo "current is Botan-3.6.1.tar.xz"
    exit 0
fi
cd /usr/local
rm -f $1
rm -rf /usr/local/include/botan
rm -rf /usr/local/lib/botan
botanBaseName=$(basename $1 .xz)
botanBaseName=$(basename $botanBaseName .tar)
rm -rf $botanBaseName
wget https://botan.randombit.net/releases/$1
tar -xJvf $1 $botanBaseName
mkdir -p /usr/local/lib/botan
cd $botanBaseName
./configure.py
make -j4
make check
make install
mv /usr/local/lib/libbotan-3.a /usr/local/lib/botan
mkdir -p /usr/local/include/botan
mv /usr/local/include/botan-3/botan/*.h /usr/local/include/botan
make clean
