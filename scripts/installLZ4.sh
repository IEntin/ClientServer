#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

cd /usr/local
git clone https://github.com/lz4/lz4.git
cd lz4
make
sudo make install
