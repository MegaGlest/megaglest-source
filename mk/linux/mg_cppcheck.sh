#!/bin/bash
# Use this script to check MegaGlest Source Code for errors using cppcheck
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

OUTFILE=./cppcheck.log

CPUS=`lscpu -p | grep -cv '^#'`
if [ "$CPUS" = '' ]; then CPUS=1; fi

cppcheck ../../source/ -i ../../source/win32_deps -i ../../source/configurator -j $CPUS --enable=all --force --verbose 2> $OUTFILE

echo "Results from cppcheck were written to $OUTFILE"
