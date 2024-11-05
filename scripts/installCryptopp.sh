#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 || $# -gt 2 ]]
then
    echo "Usage: sudo scripts/installCryptopp.sh <cryptopp release>"
    echo "current is cryptopp890.zip"
    exit 0
fi
cd /usr/local
rm -f $1
rm -rf /usr/local/include/cryptopp
rm -rf /usr/local/lib/cryptopp
cryptoppBaseName=$(basename $1 .zip)
rm -rf $cryptoppBaseName
wget https://github.com/weidai11/cryptopp/releases/download/CRYPTOPP_8_9_0/$1
unzip -aoq $1 -d $cryptoppBaseName
mkdir -p /usr/local/lib/cryptopp
cd $cryptoppBaseName
make libcryptopp.a libcryptopp.so cryptest.exe CXXFLAGS="-O3 -fPIC -pipe" -j4
cp -f libcryptopp.a /usr/local/lib/cryptopp
mkdir -p /usr/local/include/cryptopp
cp /usr/local/$cryptoppBaseName/*.h /usr/local/include/cryptopp
make clean
