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

CPU_COUNT=-1
CMAKE_ONLY=0
MAKE_ONLY=0
CLANG_FORCED=0

while getopts "c:m:n:f:h" option; do
   case "${option}" in
        c) 
           CPU_COUNT=${OPTARG}
#           echo "${option} value: ${OPTARG}"
        ;;
        m) 
           CMAKE_ONLY=${OPTARG}
#           echo "${option} value: ${OPTARG}"
        ;;
        n) 
           MAKE_ONLY=${OPTARG}
#           echo "${option} value: ${OPTARG}"
        ;;

        f) 
           CLANG_FORCED=${OPTARG}
#           echo "${option} value: ${OPTARG}"
        ;;
        h) 
                echo "Usage: $0 <option>"
                echo "       where <option> can be: -c=x, -m=1, -n=1, -f=1, -h"
                echo "       option descriptions:"
                echo "       -c=x : Force the cpu / cores count to x - example: -c=4"
                echo "       -m=1 : Force running CMAKE only to create Make files (do not compile)"
                echo "       -n=1 : Force running MAKE only to compile (assume CMAKE already built make files)"
                echo "       -f=1 : Force using CLANG compiler"
                echo "       -h   : Display this help usage"
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
CLANG_BIN_PATH=/usr/bin/

SCRIPTDIR="$(dirname $(readlink -f $0))"
cd ${SCRIPTDIR}

# Google breakpad integration (cross platform memory dumps) - OPTIONAL
# Set this to the root path of your Google breakpad subversion working copy.
# By default, this script looks for a "google-breakpad" sub-directory within
# the directory this script resides in. If this directory is not found, Cmake
# will warn about it later.
# Instead of editing this variable, consider creating a symbolic link:
#   ln -s ../../google-breakpad google-breakpad
BREAKPAD_ROOT="$(dirname $(readlink -f $0))/google-breakpad/"

# CMake options
# The default configuration works fine for regular developers and is also used 
# by our installers.
# For more cmake/build options refer to 
#   http://wiki.megaglest.org/Linux_Compiling#Building_using_CMake_by_Hand
EXTRA_CMAKE_OPTIONS=

# Build threads
# By default we use all available CPU cores to build.
# Pass '1core' as first command line argument to use only one.
NUMCORES=`lscpu -p | grep -cv '^#'`
echo "CPU cores detected: $NUMCORES"
if [ "$NUMCORES" = '' ]; then NUMCORES=1; fi
if [ $CPU_COUNT != -1 ]; then NUMCORES=$CPU_COUNT; fi
echo "CPU cores to be used: $NUMCORES"

# ----------------------------------------------------------------------------

if [ $MAKE_ONLY = 0 ]; then 
        mkdir -p build
fi

cd build
CURRENTDIR="$(dirname $(readlink -f $0))"

if [ $MAKE_ONLY = 0 ]; then 
        if [ -f 'CMakeCache.txt' ]; then rm -f 'CMakeCache.txt'; fi
fi

# Get distribution and architecture details
if [ `which lsb_release`'x' = 'x' ]
then
	lsb=0
	if [ -e /etc/debian_version ];   then distribution='Debian';   release='unknown release version'; codename=`cat /etc/debian_version`;   fi
	if [ -e /etc/SuSE-release ];     then distribution='SuSE';     release='unknown release version'; codename=`cat /etc/SuSE-release`;     fi
	if [ -e /etc/fedora-release ];   then distribution='Fedora';   release='unknown release version'; codename=`cat /etc/fedora-release`;   fi
	if [ -e /etc/redhat-release ];   then distribution='Redhat';   release='unknown release version'; codename=`cat /etc/redhat-release`;   fi
	if [ -e /etc/mandrake-release ]; then distribution='Mandrake'; release='unknown release version'; codename=`cat /etc/mandrake-release`; fi
else
	lsb=1

	# lsb_release output by example:
        #
	# $ lsb_release -i
	# Distributor ID:       Ubuntu
	#
	# $ lsb_release -d
	# Description:  Ubuntu 12.04 LTS
	#
	# $ lsb_release -r
	# Release:      12.04
	#
	# $ lsb_release -c
	# Codename:     precise

	distribution=`lsb_release -i | awk -F':' '{ gsub(/^[ \t]*/,"",$2); print $2 }'`
	release=`lsb_release -r | awk -F':' '{ gsub(/^[  \t]*/,"",$2); print $2 }'`
	codename=`lsb_release -c | awk -F':' '{ gsub(/^[ \t]*/,"",$2); print $2 }'`

	# Some distribution examples:
	#
	# OpenSuSE 11.4
	#LSB Version:    n/a
	#Distributor ID: SUSE LINUX
	#Description:    openSUSE 11.4 (x86_64)
	#Release:        11.4
	#Codename:       Celadon
	#
	# OpenSuSE 12.1
	#LSB support:  1
	#Distribution: SUSE LINUX
	#Release:      12.1
	#Codename:     Asparagus
	#
	# Arch
	#LSB Version:    n/a
	#Distributor ID: archlinux
	#Description:    Arch Linux
	#Release:        rolling
	#Codename:       n/a
	#
	# Ubuntu 12.04
	#Distributor ID: Ubuntu
	#Description:	 Ubuntu 12.04 LTS
	#Release:	 12.04
	#Codename:	 precise
fi
architecture=`uname -m`

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
esac

#exit 1;

# If, in the configuration section on top of this script, the user has 
# indicated they want to use clang in favor of the default of GCC, use clang.
if [ $CLANG_FORCED = 1 ]; then 
        EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${CLANG_BIN_PATH}clang -DCMAKE_CXX_COMPILER=${CLANG_BIN_PATH}clang++"
        echo "USER WANTS to use CLANG / LLVM compiler! EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
#exit 1;
# If both the $CC and $CXX environment variable point to something containing
# "clang", use whatever these environment variables point to.
elif [ "`echo $CC | grep -Fq 'clang'`" = 'clang' -a "`echo $CXX | grep -Fq 'clang'`" = 'clang' ]; then
	if [ `echo $CC | grep -Fq '/'` = '/' ]; then
		CLANG_CC=$CC
	else
		CLANG_CC=`which $CC`
	fi
	if [ `echo $CXX | grep -Fq '/'` = '/' ]; then
		CLANG_CXX=$CXX
	else
		CLANG_CXX=`which $CXX`
	fi
        EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${CLANG_CC} -DCMAKE_CXX_COMPILER=${CLANG_CXX}"
        echo "USER WANTS to use CLANG / LLVM compiler! EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
#exit 1;
fi

if [ $MAKE_ONLY = 0 ]; then 
        echo "Calling cmake with EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
        cmake -DCMAKE_INSTALL_PREFIX='' -DWANT_DEV_OUTPATH=ON -DWANT_STATIC_LIBS=ON -DBUILD_MEGAGLEST_TESTS=ON -DBREAKPAD_ROOT=$BREAKPAD_ROOT $EXTRA_CMAKE_OPTIONS ..
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
        echo '  mk/linux/megaglest --ini-path=mk/linux/ --data-path=mk/linux/'
        echo 'Or change into mk/linux and run it from there:'
        echo '  ./megaglest --ini-path=./ --data-path=./'
fi
