#!/bin/bash

TESTDIR=tests
BINARY=runtests

if [ -f $TESTDIR/$BINARY ]; then
    (cd $TESTDIR;./$BINARY)
else
    make -j4
fi
