#!/bin/bash
# Use this script to build MegaGlest Installer for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+


# This script is not robust for all platforms or situations. Use as a rough
#  example, but invest effort in what it's trying to do, and what it produces.
#  (make sure you don't build in features you don't need, etc).

# To install the installer silently you may run it like this:
# ./megaglest-installer.run --noprompt --i-agree-to-all-licenses --destination /home/softcoder/megaglest-temp-test --noreadme --ui=stdio

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"

# below describe various folder paths relative to the installer root folder
megaglest_linux_path="$CURRENTDIR/../.."
BINARY_DIR="$($megaglest_linux_path/make-binary-archive.sh --show-result-path2)"
DATA_DIR="$($megaglest_linux_path/make-data-archive.sh --show-result-path2)"

VERSION=`$megaglest_linux_path/mg-version.sh --version`
kernel=`uname -s | tr '[A-Z]' '[a-z]'`
architecture=`uname -m  | tr '[A-Z]' '[a-z]'`
mg_installer_bin_name=MegaGlest-Installer-${VERSION}_${architecture}_${kernel}.run

if [ "$(which zip 2>/dev/null)" = "" ]; then
    echo "Compression tool 'zip' DOES NOT EXIST on this system, please install it"; exit 1
fi
if [ "$(which xz 2>/dev/null)" = "" ]; then
    echo "Compression tool 'xz' DOES NOT EXIST on this system, please install it"; exit 1
fi
if [ "$(which tar 2>/dev/null)" = "" ]; then
    echo "Compression tool 'tar' DOES NOT EXIST on this system, please install it"; exit 1
fi

# Below is the name of the archive to create and tack onto the installer.
# *NOTE: The filename's extension is of critical importance as the installer
# does a patch on extension to figure out how to decompress!
#
#megaglest_archiver_app_data='tar -cf - * | xz > mgdata.tar.xz'
megaglest_archivefilename_data="mgdata.tar.xz"

#megaglest_archiver_app="zip -9r "
#megaglest_archivefilename="mgdata.zip"
megaglest_archiver_app="zip -0r "
megaglest_archivefilename="mgpkg.zip"
#megaglest_archiver_app="tar -c --bzip2 -f "
#megaglest_archivefilename="mgdata.tar.bz2"

# Grab the version #
#
echo "Linux project root path [$megaglest_linux_path]"

echo "About to build Installer for $VERSION"
# Stop if anything produces an error.
set -e

REPACKONLY=0
DEBUG=0
if [ "$1" = "--debug" -o "$2" = "--debug" ]; then
    echo "debug build!"
    DEBUG=1
fi
if [ "$1" = "--repackonly" -o "$2" = "--repackonly" ]; then
    echo "repacking installer only!"
    REPACKONLY=1
fi

APPNAME="MegaGlest Installer"

# I use a "cross compiler" to build binaries that are isolated from the
#  particulars of my Linux workstation's current distribution. This both
#  keeps me at a consistent ABI for generated binaries and prevent subtle
#  dependencies from leaking in.
# You may not care about this at all. In which case, just use the
#  CC=gcc and CXX=g++ lines instead.
CC="$( which gcc 2>/dev/null )"
CXX="$( which g++ 2>/dev/null )"
#CC=/opt/crosstool/gcc-3.3.6-glibc-2.3.5/i686-unknown-linux-gnu/i686-unknown-linux-gnu/bin/gcc
#CXX=/opt/crosstool/gcc-3.3.6-glibc-2.3.5/i686-unknown-linux-gnu/i686-unknown-linux-gnu/bin/g++

OSTYPE=`uname -s`
if [ "$OSTYPE" = "Linux" ]; then
    NCPU=`lscpu -p | grep -cv '^#'`
    if [ "$NUMCORES" = '' ]; then NUMCORES=1; fi
elif [ "$OSTYPE" = "Darwin" ]; then
    NCPU=`sysctl -n hw.ncpu`
elif [ "$OSTYPE" = "SunOS" ]; then
    NCPU=`/usr/sbin/psrinfo |wc -l |sed -e 's/^ *//g;s/ *$//g'`
else
    NCPU=1
fi

if [ "x$NCPU" = "x" ]; then
    NCPU=1
fi
if [ "x$NCPU" = "x0" ]; then
    NCPU=1
fi

echo "Will use make -j$NCPU. If this is wrong, check NCPU at top of script."

# Show everything that we do here on stdout.
set -x

if [ "$DEBUG" = "1" ]; then
    LUASTRIPOPT=
    BUILDTYPE=Debug
    TRUEIFDEBUG=TRUE
    FALSEIFDEBUG=FALSE
else
    LUASTRIPOPT=-s
    BUILDTYPE=MinSizeRel
    TRUEIFDEBUG=FALSE
    FALSEIFDEBUG=TRUE
fi

# Clean up previous run, build fresh dirs for Base Archive.
cd "$CURRENTDIR"
rm -rf image ${mg_installer_bin_name} ${megaglest_archivefilename}
mkdir image
mkdir image/guis
mkdir image/scripts
mkdir image/data
mkdir image/meta

# This next section copies live data from the MegaGlest directories
if [ $REPACKONLY -eq 0 ]; then
	cd "$CURRENTDIR"
	INSTALLDATADIR="data"
	rm -rf $INSTALLDATADIR
	mkdir $INSTALLDATADIR

	echo Copying MegaGlest binary files...
	$megaglest_linux_path/make-binary-archive.sh --installer
	cd "$BINARY_DIR"
	cp -r * "$CURRENTDIR/$INSTALLDATADIR"

	echo Copying MegaGlest data files...
	$megaglest_linux_path/make-data-archive.sh --installer
	cd "$DATA_DIR"
	cp -r * "$CURRENTDIR/$INSTALLDATADIR"

	cd "$CURRENTDIR"
	cp megaglest-uninstall.ico "$CURRENTDIR/$INSTALLDATADIR"
fi

if [ ! -d data/docs ]; then
    echo "We don't see data/docs ..."
    echo " Either you're in the wrong directory, or you didn't copy the"
    echo " install data into here (it's unreasonably big to store it in"
    echo " revision control for no good reason)."
    exit 1
fi


# Build MojoSetup binaries from scratch.
# YOU ALWAYS NEED THE LUA PARSER IF YOU WANT UNINSTALL SUPPORT!
#cd mojosetup
cd "$CURRENTDIR"
rm -rf cmake-build
mkdir cmake-build
cd cmake-build
# 'MOJOSETUP_GUI_*_STATIC=TRUE' > oddly it is not static after this, but isn't optional anymore so this kills portability
cmake \
    -DCMAKE_BUILD_TYPE=$BUILDTYPE \
    -DCMAKE_C_COMPILER=$CC \
    -DMOJOSETUP_MULTIARCH=FALSE \
    -DMOJOSETUP_ARCHIVE_ZIP=TRUE \
    -DMOJOSETUP_ARCHIVE_TAR=TRUE \
    -DMOJOSETUP_ARCHIVE_TAR_BZ2=TRUE \
    -DMOJOSETUP_ARCHIVE_TAR_GZ=TRUE \
    -DMOJOSETUP_ARCHIVE_TAR_XZ=TRUE \
    -DMOJOSETUP_INPUT_XZ=TRUE \
    -DMOJOSETUP_BUILD_LUAC=TRUE \
    -DMOJOSETUP_GUI_GTKPLUS2=TRUE \
    -DMOJOSETUP_GUI_GTKPLUS2_STATIC=FALSE \
    -DMOJOSETUP_GUI_NCURSES=TRUE \
    -DMOJOSETUP_GUI_NCURSES_STATIC=FALSE \
    -DMOJOSETUP_GUI_STDIO=TRUE \
    -DMOJOSETUP_GUI_STDIO_STATIC=TRUE \
    -DMOJOSETUP_GUI_WWW=FALSE \
    -DMOJOSETUP_LUALIB_DB=FALSE \
    -DMOJOSETUP_LUALIB_IO=TRUE \
    -DMOJOSETUP_LUALIB_MATH=FALSE \
    -DMOJOSETUP_LUALIB_OS=TRUE \
    -DMOJOSETUP_LUALIB_PACKAGE=TRUE \
    -DMOJOSETUP_LUA_PARSER=TRUE \
    -DMOJOSETUP_IMAGE_BMP=TRUE \
    -DMOJOSETUP_IMAGE_JPG=FALSE \
    -DMOJOSETUP_IMAGE_PNG=FALSE \
    -DMOJOSETUP_INTERNAL_BZLIB=TRUE \
    -DMOJOSETUP_INTERNAL_LIBLZMA=TRUE \
    -DMOJOSETUP_URL_HTTP=FALSE \
    -DMOJOSETUP_URL_FTP=FALSE \
    ../mojosetup

# Perhaps needed to remove compiler / linker warnings considered as errors
# sed -i 's/-Werror//' Makefile

make -j$NCPU

# Strip the binaries and GUI plugins, put them somewhere useful.
if [ "$DEBUG" != "1" ]; then
    strip ./mojosetup
fi

mv ./mojosetup ../${mg_installer_bin_name}
for feh in *.so *.dll *.dylib ; do
    if [ -f $feh ]; then
        if [ "$DEBUG" != "1" ]; then
            strip $feh
        fi
        mv $feh ../image/guis
    fi
done

# Compile the Lua scripts, put them in the base archive.
for feh in ../mojosetup/scripts/*.lua ; do
    feh_b="$(basename $feh)"
    ./mojoluac $LUASTRIPOPT -o ../image/scripts/${feh_b}c $feh
done

# Don't want the example config...use our's instead.
rm -f ../image/scripts/config.luac
./mojoluac $LUASTRIPOPT -o ../image/scripts/config.luac ../scripts/config.lua

# Don't want the example app_localization...use our's instead.
rm -f ../image/scripts/app_localization.luac
./mojoluac $LUASTRIPOPT -o ../image/scripts/app_localization.luac ../scripts/app_localization.lua

# Fill in the rest of the Base Archive...
cd ..

shopt -s extglob
if [ $REPACKONLY -eq 0 ]; then
    # Compress the main data archive
    cd data
    #${megaglest_archiver_app_data} ${megaglest_archivefilename_data}
    tar -cf - * | xz > ../$megaglest_archivefilename_data
    # now remove everything except for the docs folder and the data archive
    rm -rf !(docs|${megaglest_archivefilename_data})
    # now remove everything in the docs except files listed in config.lua
    cd docs
    rm -rf !(gnu_gpl_*.txt|cc-by-sa-*-unported.txt|README.txt)
    cd ..

    cd ..
    mv -f $megaglest_archivefilename_data data/
fi

cp -R data/* image/data/
cp meta/* image/meta/

# Need these scripts to do things like install menu items, etc, on Unix.
if [ "$OSTYPE" = "Linux" ]; then
    USE_XDG_UTILS=1
fi
if [ "$OSTYPE" = "SunOS" ]; then
    USE_XDG_UTILS=1
fi

if [ "x$USE_XDG_UTILS" = "x1" ]; then
    mkdir image/meta/xdg-utils
    cp $CURRENTDIR/mojosetup/meta/xdg-utils/* image/meta/xdg-utils/
    chmod a+rx image/meta/xdg-utils/*
fi

if [ "$OSTYPE" = "Darwin" ]; then
    # Build up the application bundle for Mac OS X...
    APPBUNDLE="$APPNAME.app"
    rm -rf "$APPBUNDLE"
    cp -Rv ../misc/MacAppBundleSkeleton "$APPBUNDLE"
	perl -w -pi -e 's/YOUR_APPLICATION_NAME_HERE/'"$APPNAME"'/g;' "${APPBUNDLE}/Contents/Info.plist"
    mv megaglest-installer "${APPBUNDLE}/Contents/MacOS/mojosetup"
    mv image/* "${APPBUNDLE}/Contents/MacOS/"
    rmdir image
    ibtool --compile "${APPBUNDLE}/Contents/Resources/MojoSetup.nib" ../misc/MojoSetup.xib
else

    # Make an archive of the Base Archive dirs and nuke the originals...
    cd image

# create the compressed image for the installer (this will use zip as a container)
#    zip -9r ../${megaglest_archivefilename} *
    ${megaglest_archiver_app} ../${megaglest_archivefilename} *

    cd ..
    rm -rf image
    # Append the archive to the mojosetup binary, so it's "self-extracting."
    cat ${megaglest_archivefilename} >> ./${mg_installer_bin_name}
    rm -f ${megaglest_archivefilename}
fi

# ...and that's that.
set +e
set +x
echo "Successfully built!"
ls -lha ${mg_installer_bin_name}

if [ "$DEBUG" = "1" ]; then
    echo
    echo
    echo
    echo 'ATTENTION: THIS IS A DEBUG BUILD!'
    echo " DON'T DISTRIBUTE TO THE PUBLIC."
    echo ' THIS IS PROBABLY BIGGER AND SLOWER THAN IT SHOULD BE.'
    echo ' YOU HAVE BEEN WARNED!'
    echo
    echo
    echo
fi

exit 0
