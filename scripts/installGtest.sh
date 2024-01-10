#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

apt-get install libgtest-dev
apt-get install cmake
cd /usr/src/gtest
cmake CMakeLists.txt
make
cp lib/*.a /usr/lib
