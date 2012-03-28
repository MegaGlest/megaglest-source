#!/bin/bash
# Use this script to check MegaGlest Source Code for errors using cppcheck
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

cppcheck ../../source/ -i ../../source/win32_deps -i ../../source/configurator -j 5 --enable=all --force --verbose 2> cppcheck.log

echo "Results from cppcheck were written to cppcheck.log"

