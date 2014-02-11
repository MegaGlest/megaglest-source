#!/bin/bash
#
# Use this script to build CEGUI dependencies on a number of Linux platforms
# ----------------------------------------------------------------------------
# Originally written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2014 Mark Vejvoda under GNU GPL v3.0

LANG=C

SCRIPTDIR="$(dirname $(readlink -f $0))"

# Build threads
# By default we use all physical CPU cores to build.
NUMCORES=`lscpu -p | grep -cv '^#'`
CPU_COUNT=-1


while getopts "c:h" option; do
   case "${option}" in
        c) 
           CPU_COUNT=${OPTARG}
#           echo "${option} value: ${OPTARG}"
        ;;
        h) 
                echo "Usage: $0 <option>"
                echo "       where <option> can be: -c x, -h"
                echo "       option descriptions:"
                echo "       -c x : Force the cpu / cores count to x - example: -c 4"
                echo "       -h   : Display this help usage"

        	exit 1        
        ;;

        \?)
              echo "Script Invalid option: -$OPTARG" >&2
              exit 1
        ;;
   esac
done



echo "CPU cores detected: $NUMCORES"
if [ "$NUMCORES" = '' ]; then NUMCORES=1; fi
if [ $CPU_COUNT != -1 ]; then NUMCORES=$CPU_COUNT; fi
echo "CPU cores to be used: $NUMCORES"

cd ${SCRIPTDIR}
mkdir -p build
cd build
if [ -f 'CMakeCache.txt' ]; then rm -f 'CMakeCache.txt'; fi

# You may require a bunch of deps to build cegui, some listed below:
#
# sudo apt-get install libpython2.7-dev libglm-dev libglfw-dev libboost1.53-dev-all

cmake -DCEGUI_BUILD_PYTHON_MODULES:BOOL=ON \
      -DCEGUI_BUILD_RENDERER_OGRE:BOOL=OFF \
      -DCEGUI_BUILD_RENDERER_OPENGL:BOOL=ON \
      -DCEGUI_BUILD_RENDERER_OPENGL3:BOOL=ON \
      -DCEGUI_BUILD_STATIC_CONFIGURATION:BOOL=ON \
      -DCEGUI_BUILD_XMLPARSER_EXPAT:BOOL=OFF \
      -DCEGUI_BUILD_XMLPARSER_LIBXML2:BOOL=OFF \
      -DCEGUI_BUILD_XMLPARSER_RAPIDXML:BOOL=ON \
      -DRAPIDXML_H_PATH=../../../shared_lib/include/xml/rapidxml/ \
      -DCEGUI_BUILD_XMLPARSER_XERCES:BOOL=OFF \
      -DCEGUI_OPTION_DEFAULT_XMLPARSER:STRING=RapidXMLParser \
      -DCEGUI_SAMPLES_USE_OGRE:BOOL=OFF \
      -DCEGUI_SAMPLES_USE_OPENGL:BOOL=OFF \
      -DCEGUI_STATIC_XMLPARSER_MODULE:STRING=CEGUIRapidXMLParser \
../

#      -DCEGUI_SAMPLE_DATAPATH=${SCRIPTDIR}/datafiles \
#      -DCEGUI_BUILD_LUA_GENERATOR:BOOL=ON \
#      -DCEGUI_BUILD_LUA_MODULE:BOOL=ON \
#      -DCEGUI_HAS_MINIZIP_RESOURCE_PROVIDER:BOOL=ON \
#      -DCEGUI_OPTION_SAFE_LUA_MODULE:BOOL=ON  \

if [ $? -ne 0 ]; then 
        echo 'ERROR: CMAKE failed.' >&2; exit 1
fi

echo "==================> About to call make with $NUMCORES cores... <=================="
make -j$NUMCORES
if [ $? -ne 0 ]; then
  echo 'ERROR: MAKE failed.' >&2; exit 2
fi

export CEGUI_SAMPLE_DATAPATH=${SCRIPTDIR}/datafiles

cd ${SCRIPTDIR}
# copy some required cegui files to the source lcoation
cp build/cegui/include/CEGUI/* cegui/include/CEGUI/

