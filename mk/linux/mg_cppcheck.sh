#!/bin/bash
# Use this script to check MegaGlest Source Code for errors using cppcheck
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

if ! cppcheck_loc="$(type -p "cppcheck")" || [ -z "$cppcheck_loc" ]; then
    # install cppcheck
    echo "CPPCHECK is not installed, installing now..."
    sudo apt install cppcheck
fi

# (Actual) Location of the cppcheck binary
CPPCHECK=$(readlink -f $(which cppcheck))

# cppcheck now depends on this library (see 'cppcheck --help' for the '--library' option)
# If you use the Debian / Ubuntu package set this to: /usr/share/cppcheck/cfg/std.cfg
if [ -e "$(dirname $CPPCHECK)/cfg/std.cfg" ]; then 
    CPPCHECKLIB=$(dirname $CPPCHECK)/cfg/std.cfg
elif [ -e "/usr/share/cppcheck/cfg/std.cfg" ]; then 
    CPPCHECKLIB=/usr/share/cppcheck/cfg/std.cfg
fi


# File to write results to
LOGFILE=/tmp/cppcheck.log

$CPPCHECK ../../source/ \
  -i ../../source/win32_deps \
  -i ../../source/configurator \
  -i ../../source/shared_lib/sources/libircclient \
  -i ../../source/shared_lib/sources/platform/miniupnpc \
  -i ../../source/shared_lib/sources/streflop \
  --library=$CPPCHECKLIB \
  --enable=all \
  --force \
  --verbose \
  2> $LOGFILE

echo "Results from cppcheck were written to $LOGFILE"
