#! /bin/sh
# Use this script to track memory use and errors while running MegaGlest
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

exec 3>&1
export GLIBCPP_FORCE_NEW=1
export GLIBCXX_FORCE_NEW=1
export G_SLICE=always-malloc

exec valgrind --num-callers=20 \
           --leak-check=yes \
           --leak-resolution=high \
           --show-reachable=yes \
           "$@" 2>&1 1>&3 3>&- |
sed 's/^==[0-9]*==/==/' >&2 1>&2 3>&-

