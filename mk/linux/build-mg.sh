#!/bin/bash
# Use this script to build MegaGlest using cmake
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011-2013 Mark Vejvoda under GNU GPL v3.0+

# ----------------------------------------------------------------------------

#
# Configuration section
#

# Default to English language output so we can understand your bug reports
export LANG=C

SCRIPTDIR="$(dirname $(readlink -f $0))"
CPU_COUNT=-1
CMAKE_ONLY=0
MAKE_ONLY=0
CLANG_FORCED=0
WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=ON"
LUA_FORCED_VERSION=0
FORCE_32BIT_CROSS_COMPILE=0

while getopts "c:dfhl:mnx" option; do
   case "${option}" in
        c) 
           CPU_COUNT=${OPTARG}
#           echo "${option} value: ${OPTARG}"
        ;;
        d) 
           WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=OFF"
#           echo "${option} value: ${OPTARG}"
        ;;
        f) 
           CLANG_FORCED=1
#           echo "${option} value: ${OPTARG}"
        ;;
        h) 
                echo "Usage: $0 <option>"
                echo "       where <option> can be: -c x, -d, -f, -m, -n, -h, -l x, -x"
                echo "       option descriptions:"
                echo "       -c x : Force the cpu / cores count to x - example: -c 4"
                echo "       -d   : Force DYNAMIC compile (do not want static libs)"
                echo "       -f   : Force using CLANG compiler"
                echo "       -l x : Force using LUA version x - example: -l 51"                
                echo "       -m   : Force running CMAKE only to create Make files (do not compile)"
                echo "       -n   : Force running MAKE only to compile (assume CMAKE already built make files)"
                echo "       -x   : Force cross compiling on x64 linux to produce an x86 32 bit binary"

                echo "       -h   : Display this help usage"

        	exit 1        
        ;;
        l) 
           LUA_FORCED_VERSION=${OPTARG}
#           echo "${option} value: ${OPTARG} LUA_FORCED_VERSION [${LUA_FORCED_VERSION}]"
        ;;
        m) 
           CMAKE_ONLY=1
#           echo "${option} value: ${OPTARG}"
        ;;
        n) 
           MAKE_ONLY=1
#           echo "${option} value: ${OPTARG}"
        ;;
        x) 
           FORCE_32BIT_CROSS_COMPILE=1
#           echo "${option} value: ${OPTARG}"
        ;;

        \?)
              echo "Script Invalid option: -$OPTARG" >&2
              exit 1
        ;;
   esac
done

#echo "CPU_COUNT = ${CPU_COUNT} CMAKE_ONLY = ${CMAKE_ONLY} CLANG_FORCED = ${CLANG_FORCED}"
#exit;

# Compiler selection
# Unless both the CC and CXX environment variables point to clang and clang++
# respectively, we use GCC. To enforce clang compilation: 
# 1. Install clang (sudo apt-get install clang)
# 2. Set the two vars below:
#    WANT_CLANG=YES and CLANG_BIN_PATH=<path_to_the_clang_binary>
CLANG_BIN_PATH=$( which clang 2>/dev/null )
CLANGPP_BIN_PATH=$( which clang++ 2>/dev/null )

cd ${SCRIPTDIR}

# Google breakpad integration (cross platform memory dumps) - OPTIONAL
# Set this to the root path of your Google breakpad subversion working copy.
# By default, this script looks for a "google-breakpad" sub-directory within
# the directory this script resides in. If this directory is not found, Cmake
# will warn about it later.
# Instead of editing this variable, consider creating a symbolic link:
#   ln -s ../../google-breakpad google-breakpad
BREAKPAD_ROOT="$SCRIPTDIR/../../google-breakpad/"

# CMake options
# The default configuration works fine for regular developers and is also used 
# by our installers.
# For more cmake/build options refer to 
#   http://wiki.megaglest.org/Linux_Compiling#Building_using_CMake_by_Hand
EXTRA_CMAKE_OPTIONS=

# Build threads
# By default we use all physical CPU cores to build.
NUMCORES=`lscpu -p | grep -cv '^#'`
echo "CPU cores detected: $NUMCORES"
if [ "$NUMCORES" = '' ]; then NUMCORES=1; fi
if [ $CPU_COUNT != -1 ]; then NUMCORES=$CPU_COUNT; fi
echo "CPU cores to be used: $NUMCORES"

# ----------------------------------------------------------------------------

# Load shared functions

. $SCRIPTDIR/mg_shared.sh

# ----------------------------------------------------------------------------

if [ $MAKE_ONLY = 0 ]; then 
        mkdir -p build
fi

cd build

if [ $MAKE_ONLY = 0 ]; then 
        if [ -f 'CMakeCache.txt' ]; then rm -f 'CMakeCache.txt'; fi
fi

# Included from shared functions
detect_system

echo 'We have detected the following system:'
echo ' [ '"$distribution"' ] [ '"$release"' ] [ '"$codename"' ] [ '"$architecture"' ]'


case $distribution in
	SuSE|SUSE?LINUX|Opensuse) 
		case $release in
			*)
				echo 'Turning ON dynamic CURL ...'
				EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_CURL_DYNAMIC_LIBS=ON"
				;;
		esac
		;;

	Fedora) 
		case $release in
			*)
				echo 'Turning ON dynamic CURL ...'
				EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_CURL_DYNAMIC_LIBS=ON"
				;;
		esac
		;;
	Arch)
	echo 'Turning ON dynamic LIBS ...'
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DWANT_STATIC_LIBS=OFF"
	;;
esac

#exit 1;

# If, in the configuration section on top of this script, the user has 
# indicated they want to use clang in favor of the default of GCC, use clang.
if [ $CLANG_FORCED = 1 ]; then 
        EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${CLANG_BIN_PATH} -DCMAKE_CXX_COMPILER=${CLANGPP_BIN_PATH}"
        echo "USER WANTS to use CLANG / LLVM compiler! EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
#exit 1;
# If both the $CC and $CXX environment variable point to something containing
# "clang", use whatever these environment variables point to.
elif [ "`echo $CC | grep -oF 'clang'`" = 'clang' -a "`echo $CXX | grep -oF 'clang'`" = 'clang' ]; then
	if [ "`echo $CC | grep -Fo '/'`" = '/' ]; then
		CLANG_CC=$CC
	else
		CLANG_CC=`which $CC`
	fi
	if [ "`echo $CXX | grep -Fo '/'`" = '/' ]; then
		CLANG_CXX=$CXX
	else
		CLANG_CXX=`which $CXX`
	fi
        EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${CLANG_CC} -DCMAKE_CXX_COMPILER=${CLANG_CXX}"
        echo "USER WANTS to use CLANG / LLVM compiler! EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
#exit 1;
fi

LUA_FORCED_CMAKE=
if [ $LUA_FORCED_VERSION != 0 ]; then
        if [ $LUA_FORCED_VERSION = 52 ]; then  
                EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_LUA_5_2=ON"
                echo "USER WANTS TO FORCE USE of LUA 5.2"
        elif [ $LUA_FORCED_VERSION = 51 ]; then 
                EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_LUA_5_1=ON"
                echo "USER WANTS TO FORCE USE of LUA 5.1"
        fi
fi

if [ $FORCE_32BIT_CROSS_COMPILE != 0 ]; then
        EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_TOOLCHAIN_FILE=../mk/cmake/Modules/Toolchain-linux32.cmake"

#LIBDIR_32bit='/usr/lib32/'
#export LD_LIBRARY_PATH="${LIBDIR_32bit}:${LD_LIBRARY_PATH}"

fi

if [ $MAKE_ONLY = 0 ]; then 
        echo "Calling cmake with EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
        cmake -DCMAKE_INSTALL_PREFIX='' -DWANT_DEV_OUTPATH=ON $WANT_STATIC_LIBS -DBUILD_MEGAGLEST_TESTS=ON -DBREAKPAD_ROOT=$BREAKPAD_ROOT $EXTRA_CMAKE_OPTIONS ../../..
        if [ $? -ne 0 ]; then 
          echo 'ERROR: CMAKE failed.' >&2; exit 1
        fi
fi

if [ $CMAKE_ONLY = 1 ]; then 
        echo "==================> You may now call make with $NUMCORES cores... <=================="
else
        echo "==================> About to call make with $NUMCORES cores... <=================="
        make -j$NUMCORES
        if [ $? -ne 0 ]; then
          echo 'ERROR: MAKE failed.' >&2; exit 2
        fi

        cd ..
        echo ''
        echo 'BUILD COMPLETE.'
        echo ''
        echo 'To launch MegaGlest from the current directory, use:'
        echo '  ./megaglest'
        echo 'Or change into mk/linux and run it from there:'
        echo '  ./megaglest --ini-path=./ --data-path=./'
fi

