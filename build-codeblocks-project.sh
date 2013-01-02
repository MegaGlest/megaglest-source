#!/bin/bash
# Use this script to build MegaGlest CodeBlocks project files from CMake
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2013 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname $(readlink -f $0))"
mkdir -p codeblocks
cd codeblocks

if [ -f 'CMakeCache.txt' ]; then rm -f 'CMakeCache.txt'; fi

LANG=C
NUMCORES=`cat /proc/cpuinfo | grep -cE '^processor'`

# This is for regular developers and used by our installer
cmake -G"CodeBlocks - Unix Makefiles" -DCMAKE_INSTALL_PREFIX= -DWANT_STATIC_LIBS=ON -DCMAKE_ECLIPSE_MAKE_ARGUMENTS=-j$NUMCORES ${CURRENTDIR}
if [ $? -ne 0 ]; then 
  echo 'ERROR: CMAKE failed.' >&2; exit 1
fi

cd ${CURRENTDIR}
echo ''
echo '*** CodeBlocks Project files [MegaGlest.cbp] can be opened from the folder: [codeblocks]'
echo 'detected and using: ' $NUMCORES ' cores...'
echo 'Browse where your build tree is ['${CURRENTDIR}'/codeblocks] and select the project.'
echo 'You get a fully functional CodeBlocks project'
echo ''
ls -la codeblocks
