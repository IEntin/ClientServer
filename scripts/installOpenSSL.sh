#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

if [[ ( $@ == "--help") ||  $@ == "-h" ]]
then
    echo "Usage: sudo scripts/installOpenSSL.sh"
    exit 0
fi

apt install libssl-dev
