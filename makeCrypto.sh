#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 || $# -gt 2 ]]
then
    echo "Usage: ./makeCrypto.sh <required cryptopp release>"
    echo "current is cryptopp870.zip"
    exit 0
fi

(
wget https://www.cryptopp.com/$1
unzip -aoq cryptopp870.zip -d cryptopp
cd cryptopp
CXX=clang++ make CXXFLAGS="-O3 -fPIC -pipe" -j4
cp libcryptopp.a libcryptoppclang.a
make clean
CXX=g++ make CXXFLAGS="-O3 -fPIC -pipe" -j4
cp libcryptopp.a libcryptoppgcc.a
make clean
)
