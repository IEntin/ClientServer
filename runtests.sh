#!/bin/bash

make -j4

TESTDIR=tests

(cd $TESTDIR;./runtests)
