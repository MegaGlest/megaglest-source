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
GCC_FORCED=0
WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=ON"
FORCE_EMBEDDED_LIBS=0
LUA_FORCED_VERSION=0
COMPILATION_WITHOUT=0

while getopts "c:defhl:mnwxb" option; do
	case "${option}" in
		c) CPU_COUNT=${OPTARG};;
		d) WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=OFF";;
		e) FORCE_EMBEDDED_LIBS=1;;
		f) GCC_FORCED=1;;
		h) 	echo "Usage: $0 <option>"
			echo "       where <option> can be: -b, -c x, -d, -e, -f, -m, -n, -h, -l x, -w, -x"
			echo "       option descriptions:"
			echo "       -b   : Force default configuration designed for bundle/release."
			echo "       -c x : Force the cpu / cores count to x - example: -c 4"
			echo "       -d   : Force DYNAMIC compile (do not want static libs)"
			echo "       -e   : Force compile with EMBEDDED libraries"
			echo "       -f   : Force using Gcc compiler"
			echo "       -l x : Force using LUA version x - example: -l 5.3"
			echo "       -m   : Force running CMAKE only to create Make files (do not compile)"
			echo "       -n   : Force running MAKE only to compile (assume CMAKE already built make files)"
			echo "       -w   : Force compilation 'Without using wxWidgets'"
			echo "       -x   : Force usage of Xcode and xcodebuild"
			echo "       -h   : Display this help usage"
			exit 0;;
		l) LUA_FORCED_VERSION=${OPTARG};;
		m) CMAKE_ONLY=1;;
		n) MAKE_ONLY=1;;
		w) COMPILATION_WITHOUT=1;;
		x) USE_XCODE=1;;
		b)	BUILD_BUNDLE=1
			#CPU_COUNT=-1
			CMAKE_ONLY=0
			MAKE_ONLY=0
			USE_XCODE=0
			GCC_FORCED=0
			WANT_STATIC_LIBS="-DWANT_STATIC_LIBS=ON"
			LUA_FORCED_VERSION=0
			COMPILATION_WITHOUT=1;;
		\?)
			echo "Script Invalid option: -$OPTARG" >&2
			exit 1;;
   esac
done

CLANG_BIN_PATH="$(which clang 2>/dev/null)"
CLANGPP_BIN_PATH="$(which clang++ 2>/dev/null)"
GCC_BIN_PATH="/opt/local/bin/gcc"
GCCPP_BIN_PATH="/opt/local/bin/g++"
CMAKE_BIN_PATH="$(which cmake 2>/dev/null)"
if [ "$CMAKE_BIN_PATH" = "" ]; then CMAKE_BIN_PATH="/opt/local/bin/cmake"; fi
# ^ install latest (not beta) gcc from "mac ports" and then choose it as default gcc version by "port select ..."
# ( ^ same situation is with wxwidgets )
cd ${SCRIPTDIR}

if [ "$BUILD_BUNDLE" -eq "1" ] && [ -d "p7zip" ]; then rm -rf "p7zip"; fi
if [ -e ".p7zip.zip" ] && [ "$(find ./ -name ".p7zip.zip" -mtime +90)" ]; then rm ".p7zip.zip"; rm -rf "p7zip"; fi
if [ ! -e ".p7zip.zip" ]; then
	curl -L -o .p7zip.zip https://github.com/MegaGlest/megaglest-source/releases/download/3.3.0/p7zip.zip 2>/dev/null
	# ^sha256: 20ac3b0377054f8196c10e569bd6ec7c6ed06d519fa39e781ee6d27d7887588b
	if [ -e ".p7zip.zip" ]; then touch -m ".p7zip.zip"; fi
fi
if [ ! -d "p7zip" ]; then unzip .p7zip.zip >/dev/null; fi

if [ "$BUILD_BUNDLE" -eq "1" ]; then
	if [ -e "megaglest" ] && [ "$(./megaglest --version >/dev/null; echo "$?")" -eq "0" ]; then
		if [ -d "lib" ]; then rm -rf "lib"; fi
		mkdir -p "lib"
		list_of_libs="$(otool -L megaglest | grep -v '/System/Library/Frameworks/' | grep -v '/usr/lib/' | awk '{print $1}' | sed '/:$/d')"
		for (( i=1; i<=50; i++ )); do
		    for dyn_lib in $list_of_libs; do
			if [ "$(echo "$list_of_checked_libs" | grep "$dyn_lib")" = "" ]; then
			    list_of_libs2="$(otool -L "$dyn_lib" | grep -v '/System/Library/Frameworks/' | grep -v '/usr/lib/' | awk '{print $1}')"
			    list_of_libs="$(echo "$list_of_libs
$list_of_libs2" | sed '/:$/d' | sed '/^$/d' | sort -u )"
			    list_of_checked_libs="$list_of_checked_libs $dyn_lib"
			fi
		    done
		done
		for dyn_lib in $list_of_libs; do
		    cp "$dyn_lib" "lib/"
		done
		
		if [ "$(find lib -type f -name "libvlc.*")" != "" ]; then
			LIBVLC_DIR_CHECK="$( echo "$list_of_checked_libs" | tr ' ' '\n' | grep "libvlc\." | sort -u | head -1 )"
			if [ "$LIBVLC_DIR_CHECK" != "" ]; then
				LIBVLC_DIR="$(cd "$(dirname "$LIBVLC_DIR_CHECK")"; pwd)"
				if [ -d "$LIBVLC_DIR/vlc/plugins" ]; then
					mkdir -p "lib/vlc"
					cp -f -r "$LIBVLC_DIR/vlc/plugins" "lib/vlc/"
				fi
			fi
		fi
	else
		echo 'Error: Please run first at least once build-mg.sh script to be ready for prepare directory with dynamic libraries.'
		# strange method but required for cpack/.dmg
		exit 1
	fi
fi

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
	*) 	if [ "$WANT_STATIC_LIBS" = "-DWANT_STATIC_LIBS=ON" ]; then
			echo 'Turning ON dynamic PNG ...'
			EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DSTATIC_PNG=OFF -DWANT_USE_VLC=OFF -DWANT_USE_OpenSSL=OFF"
		fi;;
esac
case $xcode_ver in
	# en.wikipedia.org/wiki/Xcode, Version, OS X SDK(s) <- lowest, but not less than 10.4
	2.*|3.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.4";;
	4.0*|4.1*|4.2*|4.3*) CMAKE_OSX_DEPLOYMENT_TARGET="10.6";;
	4.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.7";;
	5.*) CMAKE_OSX_DEPLOYMENT_TARGET="10.8";;
	6.0*|6.1*|6.2*) CMAKE_OSX_DEPLOYMENT_TARGET="10.9";;
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
	if [ "$GCC_FORCED" -ne "1" ]; then
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${CLANG_BIN_PATH} -DCMAKE_CXX_COMPILER=${CLANGPP_BIN_PATH}"
	else
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_C_COMPILER=${GCC_BIN_PATH} -DCMAKE_CXX_COMPILER=${GCCPP_BIN_PATH}"
		echo "USER WANTS to use Gcc compiler!"
	fi
fi

if [ "$LUA_FORCED_VERSION" != "0" ] && [ "$LUA_FORCED_VERSION" != "" ]; then
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_LUA_VERSION=$LUA_FORCED_VERSION"
	#echo "USER WANTS TO FORCE USE of LUA $LUA_FORCED_VERSION"
fi

if [ "$FORCE_EMBEDDED_LIBS" != "0" ] && [ "$FORCE_EMBEDDED_LIBS" != "" ]; then
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DFORCE_EMBEDDED_LIBS=ON"
fi

if [ "$COMPILATION_WITHOUT" != "0" ] && [ "$COMPILATION_WITHOUT" != "" ]; then
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF"
fi

if [ "$MAKE_ONLY" -eq "0" ]; then
	EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DWANT_DEV_OUTPATH=ON $WANT_STATIC_LIBS -DBREAKPAD_ROOT=$BREAKPAD_ROOT"
	if [ "$BUILD_BUNDLE" -ne "1" ]; then
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCMAKE_INSTALL_PREFIX=''"
		if [ "$GCC_FORCED" -ne "1" ] || [ "$USE_XCODE" -eq "1" ]; then :
			#^ Remove this condition when it V will start working on gcc
			#EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DBUILD_MEGAGLEST_TESTS=ON"
			#^ Uncomment when it will start working on clang
		else
			rm -f ../megaglest_tests
		fi
		rm -f ../MegaGlest*.dmg
	else
		EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DCPACK_GENERATOR=Bundle -DWANT_SINGLE_INSTALL_DIRECTORY=ON"
		rm -f ../megaglest_editor ../megaglest_g3dviewer ../megaglest_tests
	fi
	echo "Calling cmake with EXTRA_CMAKE_OPTIONS = ${EXTRA_CMAKE_OPTIONS}"
	if [ "$USE_XCODE" -eq "1" ]; then
		$CMAKE_BIN_PATH -GXcode $EXTRA_CMAKE_OPTIONS ../../..
		if [ "$?" -ne "0" ]; then echo 'ERROR: CMAKE failed.' >&2; exit 1; fi
	else
		$CMAKE_BIN_PATH —G"Unix Makefiles" $EXTRA_CMAKE_OPTIONS ../../..
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
		#echo '>>> megaglest_editor --help'
		#./megaglest_editor --help | head -3
		#echo '>>> megaglest_g3dviewer --help'
		#./megaglest_g3dviewer --help | head -3
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
