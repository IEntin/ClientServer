#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ $@ == "--help" ||  $@ == "-h" ]]
then
    echo "Not yet tested"
    echo "Usage: sudo scripts/installCryptopp.sh"
    exit 0
fi

set -e

cd /tmp
wget https://github.com/cryptopp-modern/cryptopp-modern/releases/download/2026.8.1/cryptopp-modern-2026.8.1.zip
unzip cryptopp-modern-2026.8.1.zip -d cryptopp-modern
cd cryptopp-modern

make -j$(nproc)
make install PREFIX=/usr/local
sudo ldconfig
