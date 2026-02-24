#!/bin/bash

#
# Copyright (C) 2021 Ilya Entin
#

sudo apt update
sudo dpkg --configure -a
sudo apt install -f
