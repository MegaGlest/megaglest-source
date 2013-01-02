#!/bin/bash
# Use this script to build MegaGlest Eclipse project files from CMake
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname $(readlink -f $0))"
mkdir -p eclipse
cd eclipse

if [ -f 'CMakeCache.txt' ]; then rm -f 'CMakeCache.txt'; fi

LANG=C
NUMCORES=`cat /proc/cpuinfo | grep -cE '^processor'`

# This is for regular developers and used by our installer
cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_INSTALL_PREFIX= -DWANT_STATIC_LIBS=ON -DCMAKE_ECLIPSE_MAKE_ARGUMENTS=-j$NUMCORES ${CURRENTDIR}
if [ $? -ne 0 ]; then 
  echo 'ERROR: CMAKE failed.' >&2; exit 1
fi

cd ${CURRENTDIR}
echo ''
echo '*** Eclipse Project files [.project and .cproject] can be imported from the folder: [eclipse]'
echo 'detected and using: ' $NUMCORES ' cores...'
echo 'Import the created project using Menu File->Import'
echo 'Select General->Existing projects into workspace:'
echo 'Browse where your build tree is ['${CURRENTDIR}'/eclipse] and select the directory.'
echo 'Keep "Copy projects into workspace" unchecked.'
echo 'You get a fully functional eclipse project'
echo ''
ls -la eclipse
