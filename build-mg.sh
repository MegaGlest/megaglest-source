#!/bin/bash
# Use this script to build MegaGlest using cmake
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

LANG=C
NUMCORES=`cat /proc/cpuinfo | grep -cE '^processor'`

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
cmake -DCMAKE_INSTALL_PREFIX='' -DWANT_DEV_OUTPATH=ON -DWANT_STATIC_LIBS=ON -DBREAKPAD_ROOT=${CURRENTDIR}/../google-breakpad/ ${EXTRA_CMAKE_OPTIONS} ..
if [ $? -ne 0 ]; then 
  echo 'ERROR: CMAKE failed.' >&2; exit 1
fi

make -j$NUMCORES
if [ $? -ne 0 ]; then
  echo 'ERROR: MAKE failed.' >&2; exit 2
fi

cd ..
echo ''
echo 'BUILD COMPLETE.'
echo ''
echo 'To build with boogle-breakpad support pass the path to the library as follows:'
echo 'cmake -DBREAKPAD_ROOT=/home/softcoder/Code/google-breakpad/'
echo 'To launch MegaGlest from the current directory, use:'
echo '  mk/linux/megaglest --ini-path=mk/linux/ --data-path=mk/linux/'
echo 'Or change into mk/linux and run it from there:'
echo '  ./megaglest --ini-path=./ --data-path=./'
