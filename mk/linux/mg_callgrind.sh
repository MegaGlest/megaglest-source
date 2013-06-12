#! /bin/sh
# Use this script to test performance while running MegaGlest
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

exec 3>&1
export GLIBCPP_FORCE_NEW=1
export GLIBCXX_FORCE_NEW=1
export G_SLICE=always-malloc

exec valgrind --tool=callgrind \
           "$@" 2>&1 1>&3 3>&- |
sed 's/^==[0-9]*==/==/' >&2 1>&2 3>&-

echo 'Look for a generated file called callgrind.out.x.'
echo 'You can then use kcachegrind tool to read this file.'
echo 'It will give you a graphical analysis of things with results like which lines cost how much.'
