#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

tar xzvf cryptopp-CRYPTOPP_8_7_0.tar.gz

(cd cryptopp-CRYPTOPP_8_7_0; make; mv libcryptopp.a ../libs)
