#!/bin/sh
# Use this script to build MegaGlest using cmake
# ----------------------------------------------------------------------------
# 2011 Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# 2015 Rewritten by filux <heross(@@)o2.pl>
# Copyright (c) 2011-2015 under GNU GPL v3.0+
# ----------------------------------------------------------------------------
# Default to English language output so we can understand your bug reports
export LANG=C

SCRIPTDIR="$(cd "$(dirname "$0")"; pwd)"
BUILD_BUNDLE=0
CPU_COUNT=-1
CMAKE_ONLY=0
MAKE_ONLY=0
USE_XCODE=0
CLANG_FORCED=0
WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=ON"
LUA_FORCED_VERSION=0

while getopts "c:dfhl:mnxb" option; do
	case "${option}" in
		c) CPU_COUNT=${OPTARG};;
		d) WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=OFF";;
		f) CLANG_FORCED=1;;
		h) 	echo "Usage: $0 <option>"
			echo "       where <option> can be: -b, -c x, -d, -f, -m, -n, -h, -l x, -x"
			echo "       option descriptions:"
			echo "       -b   : Force default configuration designed for bundle/release."
			echo "       -c x : Force the cpu / cores count to x - example: -c 4"
			echo "       -d   : Force DYNAMIC compile (do not want static libs)"
			echo "       -f   : Force using Clang compiler"
			echo "       -l x : Force using LUA version x - example: -l 51"
			echo "       -m   : Force running CMAKE only to create Make files (do not compile)"
			echo "       -n   : Force running MAKE only to compile (assume CMAKE already built make files)"
			echo "       -x   : Force usage of Xcode and xcodebuild"
			echo "       -h   : Display this help usage"
			exit 0;;
		l) LUA_FORCED_VERSION=${OPTARG};;
		m) CMAKE_ONLY=1;;
		n) MAKE_ONLY=1;;
		x) USE_XCODE=1;;
		b)	BUILD_BUNDLE=1
			#CPU_COUNT=-1
			CMAKE_ONLY=0
			MAKE_ONLY=0
			USE_XCODE=0
			CLANG_FORCED=0
			WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=ON"
			LUA_FORCED_VERSION=0;;
		\?)
			echo "Script Invalid option: -$OPTARG" >&2
			exit 1;;
   esac
done

CLANG_BIN_PATH="$(which clang 2>/dev/null)"
CLANGPP_BIN_PATH="$(which clang++ 2>/dev/null)"
GCC_BIN_PATH="/opt/local/bin/gcc"
GCCPP_BIN_PATH="/opt/local/bin/g++"
# ^ install latest (not beta) gcc from "mac ports" and then choose it as default gcc version by "port select ..."
# ( ^ same situation is with wxwidgets )
cd ${SCRIPTDIR}

if [ "$BUILD_BUNDLE" -eq "1" ] && [ -d "p7zip" ]; then rm -rf "p7zip"; fi
if [ -e ".p7zip.zip" ] && [ "$(find ./ -name ".p7zip.zip" -mtime +90)" ]; then rm ".p7zip.zip"; rm -rf "p7zip"; fi
if [ ! -e ".p7zip.zip" ]; then
	curl -L -o .p7zip.zip https://github.com/MegaGlest/megaglest-source/releases/download/3.2.3/p7zip.zip 2>/dev/null
	if [ -e ".p7zip.zip" ]; then touch -m ".p7zip.zip"; fi
fi
if [ ! -d "p7zip" ]; then unzip .p7zip.zip >/dev/null; fi

# Google breakpad integration (cross platform memory dumps) - OPTIONAL
# Set this to the root path of your Google breakpad subversion working copy.
# By default, this script looks for a "google-breakpad" sub-directory within
# the directory this script resides in. If this directory is not found, Cmake
# will warn about it later.
# Instead of editing this variable, consider creating a symbolic link:
#   ln -s ../../google-breakpad google-breakpad
BREAKPAD_ROOT="$SCRIPTDIR/../../google-breakpad/"

# Build threads
# By default we use all physical CPU cores to build.
NUMCORES="$(sysctl -n hw.ncpu)"
echo "CPU cores detected: $NUMCORES"
if [ "$NUMCORES" = '' ]; then NUMCORES=1; fi
if [ "$CPU_COUNT" != -1 ]; then NUMCORES=$CPU_COUNT; fi
echo "CPU cores to be used: $NUMCORES"

if [ "$BUILD_BUNDLE" -eq "1" ] && [ -d "build" ]; then rm -rf build; fi
if [ $MAKE_ONLY = 0 ]; then mkdir -p build; fi
cd build

if [ $MAKE_ONLY = 0 ] && [ -f 'CMakeCache.txt' ]; then rm -f 'CMakeCache.txt'; fi

distribution="$(sw_vers -productName)"
release="$(sw_vers -productVersion)"
xcode_ver="$(xcodebuild -version | awk '/Xcode/ {print $2}')"
architecture="$(uname -m)"
echo 'We have detected the following system:'
echo " [ $distribution ] [ $release ] [ $architecture ] [ $xcode_ver ]"
case $release in
	*) 	echo 'Turning ON dynamic PNG ...'
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DPNG_STATIC=OFF";;
esac
case $xcode_ver in
	# en.wikipedia.org/wiki/Xcode, Version, OS X SDK(s) <- lowest, but not less than 10.4
	2.*|3.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.4";;
	4.0*|4.1*|4.2*|4.3*) CMAKE_OSX_DEPLOYMENT_TARGET="10.6";;
	4.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.7";;
	5.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.8";;
	6.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.9";;
	7.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.10";;
  	# ^ last one expected for future
esac
case $CMAKE_OSX_DEPLOYMENT_TARGET in
	10.4*|10.5*) CMAKE_OSX_ARCHITECTURES="ppc;i386";;
	10.6*) CMAKE_OSX_ARCHITECTURES="i386;x86_64";;
esac
if [ "$CMAKE_OSX_DEPLOYMENT_TARGET" != "" ]; then
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
fi
if [ "$CMAKE_OSX_ARCHITECTURES" != "" ]; then
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
fi

if [ "$USE_XCODE" -ne "1" ]; then
	if [ "$CLANG_FORCED" -eq "1" ]; then
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${CLANG_BIN_PATH} -DCMAKE_CXX_COMPILER=${CLANGPP_BIN_PATH}"
		echo "USER WANTS to use CLANG / LLVM compiler!"
	else
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${GCC_BIN_PATH} -DCMAKE_CXX_COMPILER=${GCCPP_BIN_PATH}"
	fi
fi

LUA_FORCED_CMAKE=
if [ "$LUA_FORCED_VERSION" -ne "0" ]; then
	if [ "$LUA_FORCED_VERSION" -eq "52" ]; then
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_LUA_5_2=ON"
		echo "USER WANTS TO FORCE USE of LUA 5.2"
	elif [ "$LUA_FORCED_VERSION" -eq "51" ]; then
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_LUA_5_1=ON"
		echo "USER WANTS TO FORCE USE of LUA 5.1"
	fi
fi

if [ "$MAKE_ONLY" -eq "0" ]; then
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DWANT_DEV_OUTPATH=ON $WANT_STATIC_LIBS -DBREAKPAD_ROOT=$BREAKPAD_ROOT"
	if [ "$BUILD_BUNDLE" -ne "1" ]; then
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_INSTALL_PREFIX=''"
		if [ "$CLANG_FORCED" -eq "1" ] || [ "$USE_XCODE" -eq "1" ]; then
			#^ Remove this condition when it V will start working on gcc
			EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DBUILD_MEGAGLEST_TESTS=ON"
		else
			rm -f ../megaglest_tests
		fi
		rm -f ../MegaGlest*.dmg
	else
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCPACK_GENERATOR=Bundle -DSINGLE_INSTALL_DIRECTORY=ON -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF"
		rm -f ../megaglest_editor ../megaglest_g3dviewer ../megaglest_tests
	fi
	echo "Calling cmake with EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
	if [ "$USE_XCODE" -eq "1" ]; then
		cmake -GXcode $EXTRA_CMAKE_OPTIONS ../../..
		if [ "$?" -ne "0" ]; then echo 'ERROR: CMAKE failed.' >&2; exit 1; fi
	else
		cmake —G"Unix Makefiles" $EXTRA_CMAKE_OPTIONS ../../..
		if [ "$?" -ne "0" ]; then echo 'ERROR: CMAKE failed.' >&2; exit 1; fi
	fi
fi

if [ "$CMAKE_ONLY" -eq "1" ]; then
	echo "==================> You may now call make with $NUMCORES cores... <=================="
else
	if [ "$USE_XCODE" -eq "1" ]; then
		echo "==================> About to call xcodebuild... <================================="
		xcodebuild | egrep "(error|warning):"
		if [ "$?" -ne "0" ]; then echo 'ERROR: xcodebuild failed.' >&2; exit 2; fi
	else
		echo "==================> About to call make with $NUMCORES cores... <=================="
		make -j$NUMCORES
		if [ "$?" -ne "0" ]; then echo 'ERROR: MAKE failed.' >&2; exit 2; fi
	fi

	if [ -d "../Debug" ]; then mv -f ../Debug/megaglest* "$SCRIPTDIR"; rm -rf ../Debug
	elif [ -d "../Release" ]; then mv -f ../Release/megaglest* "$SCRIPTDIR"; rm -rf ../Release; fi

	cd ..
	if [ "$BUILD_BUNDLE" -ne "1" ]; then
		echo ''
		echo 'BUILD COMPLETE.'
		echo ''
		echo '- - - - - - - - - - - - - - - - - - - -'
		echo 'Mini test:'
		echo '>>> megaglest --version'
		./megaglest --version | head -3
		echo '>>> megaglest_editor --help'
		./megaglest_editor --help | head -3
		echo '>>> megaglest_g3dviewer --help'
		./megaglest_g3dviewer --help | head -3
		echo 'Dependencies:'
		otool -L megaglest
		echo '- - - - - - - - - - - - - - - - - - - -'
		echo ''
		echo 'To launch MegaGlest from the current directory, use:'
		echo '  ./megaglest'
		echo ''
	else
		ls -lhA megaglest
		echo ''
		./megaglest --version
		echo ''
	fi
fi
