#!/bin/bash

make -j4 buildtests
(cd test_bin;./runtests)
