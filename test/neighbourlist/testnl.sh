#!/bin/bash
# Script that runs the tests of the mac buffer
# USAGE: ./testmacbuffer.sh

# copy mac buffer files and compile
make

# run test
./macbuffertest
