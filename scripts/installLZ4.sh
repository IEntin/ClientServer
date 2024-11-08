#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

cd /usr/local
git clone https://github.com/lz4/lz4.git
cd lz4
make
sudo make install
sudo mkdir -p /usr/local/lib/lz4
sudo cp /usr/local/lib/liblz4.a /usr/local/lib/lz4/liblz4.a

