#!/bin/bash
# Use this script to build MegaGlest using cmake
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011-2013 Mark Vejvoda under GNU GPL v3.0+

# to enable clang compilation: 
# 1. sudo apt-get install clang
# 2. Set the two vars below, WANT_CLANG=YES and CLANG_BIN_PATH=<path to the clang binary>
#    OR: set both the CC and CXX environment variables to point to clang and clang++ respectively
WANT_CLANG=NO
CLANG_BIN_PATH=/usr/bin/

LANG=C
NUMCORES=`lscpu -p | grep -cv '^#'`
if [ "$NUMCORES" = '' ]; then NUMCORES=1; fi
if [ "$1" = '1core' ]; then NUMCORES=1; fi

mkdir -p build
cd build

if [ -f 'CMakeCache.txt' ]; then rm -f 'CMakeCache.txt'; fi

# This is for regular developers and used by our installer
# For more cmake/build options refer to 
# http://wiki.megaglest.org/Linux_Compiling#Building_using_CMake_by_Hand
# this script looks for google-breakpad in the main root folder, you may link to the real path using:
# ln -s ../../google-breakpad/ google-breakpad

LANG=C
svnversion=`readlink -f $0 | xargs dirname | xargs svnversion`
architecture=`uname -m`

# Is the lsb_release command supported?
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

echo 'We have detected the following system:'
echo ' [ '"$distribution"' ] [ '"$release"' ] [ '"$codename"' ] [ '"$architecture"' ]'

EXTRA_CMAKE_OPTIONS=

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

CURRENTDIR="$(dirname $(readlink -f $0))"

if [ "$WANT_CLANG" = 'YES' -o "`echo $CXX | grep -Fq 'clang'`" = 'clang' ]; then 
        EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${CLANG_BIN_PATH}clang -DCMAKE_CXX_COMPILER=${CLANG_BIN_PATH}clang++"
        echo "USER WANTS to use CLANG / LLVM compiler! EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
#exit 1;
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

echo "Calling cmake with EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
cmake -DCMAKE_INSTALL_PREFIX='' -DWANT_DEV_OUTPATH=ON -DWANT_STATIC_LIBS=ON -DBUILD_MEGAGLEST_TESTS=ON -DBREAKPAD_ROOT=${CURRENTDIR}/../google-breakpad/ ${EXTRA_CMAKE_OPTIONS} ..
if [ $? -ne 0 ]; then 
  echo 'ERROR: CMAKE failed.' >&2; exit 1
fi

echo "==================> About to call make with $NUMCORES cores... <=================="
make -j$NUMCORES
if [ $? -ne 0 ]; then
  echo 'ERROR: MAKE failed.' >&2; exit 2
fi

cd ..
echo ''
echo 'BUILD COMPLETE.'
echo ''
echo 'To build with google-breakpad support pass the path to the library as follows:'
echo 'cmake -DBREAKPAD_ROOT=/home/softcoder/Code/google-breakpad/'
echo 'To launch MegaGlest from the current directory, use:'
echo '  mk/linux/megaglest --ini-path=mk/linux/ --data-path=mk/linux/'
echo 'Or change into mk/linux and run it from there:'
echo '  ./megaglest --ini-path=./ --data-path=./'

