#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" || $# -lt 1 || $# -gt 2 ]]
then
    echo "Usage: ./makeCrypto.sh <required cryptopp release>"
    echo "current is cryptopp880.zip"
    exit 0
fi

(
    cd /usr/local
    wget -O https://www.cryptopp.com/$1
    cryptoppBaseName=$(basename $1 .zip)
    echo "$cryptoppBaseName"
    unzip -aoq $1 -d "$cryptoppBaseName"
    cd "$cryptoppBaseName"
    CXX=clang++ make CXXFLAGS="-O3 -fPIC -pipe" -j4
    cp -f libcryptopp.a /usr/local/lib/libcryptoppclang.a
    make clean
    CXX=g++ make CXXFLAGS="-O3 -fPIC -pipe" -j4
    cp -f libcryptopp.a /usr/local/lib/libcryptoppgcc.a
    make clean
    ln -sf /usr/local/"$cryptoppBaseName" /usr/local/include/cryptopp
)
