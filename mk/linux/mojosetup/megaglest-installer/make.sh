#!/bin/bash

# This script is not robust for all platforms or situations. Use as a rough
#  example, but invest effort in what it's trying to do, and what it produces.
#  (make sure you don't build in features you don't need, etc).

# below is the branch to build and installer from
megaglest_release_folder="trunk"
#megaglest_release_folder="release-3.3.4.1"

# below describe various folder paths relative to the installer root folder
megaglest_project_root=../../../../../
megaglest_data_path=${megaglest_project_root}${megaglest_release_folder}/data/glest_game/
megaglest_linux_path=${megaglest_project_root}${megaglest_release_folder}/mk/linux/

# Below is the name of the archive to create and tack onto the installer.
# *NOTE: The filename's extension is of critical importance as the installer
# does a patch on extension to figure out how to decompress!
#
# static const MojoArchiveType archives[] =
# {
#    { "zip", MojoArchive_createZIP, true },
#    { "tar", MojoArchive_createTAR, true },
#    { "tar.gz", MojoArchive_createTAR, true },
#    { "tar.bz2", MojoArchive_createTAR, true },
#    { "tgz", MojoArchive_createTAR, true },
#    { "tbz2", MojoArchive_createTAR, true },
#    { "tb2", MojoArchive_createTAR, true },
#    { "tbz", MojoArchive_createTAR, true },
#    { "uz2", MojoArchive_createUZ2, false },
#    { "pck", MojoArchive_createPCK, true },
# };
#
megaglest_archiver_app="zip -9r "
megaglest_archivefilename="mgdata.zip"
#megaglest_archiver_app="tar -c --bzip2 -f "
#megaglest_archivefilename="mgdata.tar.bz2"

# Grab the version # from autoconf
#
pushd "`pwd`/${megaglest_linux_path}"
echo "Linux project root path [`pwd`/${megaglest_linux_path}]"
VERSION=`autoconf -t AC_INIT | sed -e 's/[^:]*:[^:]*:[^:]*:[^:]*:\([^:]*\):.*/\1/g'`
echo "About to build Installer for $VERSION"
popd

# Stop if anything produces an error.
set -e

REPACKONLY=0
DEBUG=0
if [ "$1" = "--debug" ]; then
    echo "debug build!"
    DEBUG=1
elif [ "$1" = "--repackonly" ]; then
    echo "reacking installer only!"
    REPACKONLY=1
fi

APPNAME="MegaGlest Installer"

# I use a "cross compiler" to build binaries that are isolated from the
#  particulars of my Linux workstation's current distribution. This both
#  keeps me at a consistent ABI for generated binaries and prevent subtle
#  dependencies from leaking in.
# You may not care about this at all. In which case, just use the
#  CC=gcc and CXX=g++ lines instead.
CC=/usr/bin/gcc
CXX=/usr/bin/g++
#CC=/opt/crosstool/gcc-3.3.6-glibc-2.3.5/i686-unknown-linux-gnu/i686-unknown-linux-gnu/bin/gcc
#CXX=/opt/crosstool/gcc-3.3.6-glibc-2.3.5/i686-unknown-linux-gnu/i686-unknown-linux-gnu/bin/g++

OSTYPE=`uname -s`
if [ "$OSTYPE" = "Linux" ]; then
    NCPU=`cat /proc/cpuinfo |grep vendor_id |wc -l`
    let NCPU=$NCPU+1
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
rm -rf image megaglest-installer ${megaglest_archivefilename}
mkdir image
mkdir image/guis
mkdir image/scripts
mkdir image/data
mkdir image/meta

# This next section copies live data from the mega-glest folders
if [ $REPACKONLY -eq 0 ]; then

	rm -rf data
	mkdir data
	mkdir data/bin

	INSTALL_ROOTDIR="`pwd`/"
	INSTALLDATADIR="${INSTALL_ROOTDIR}data/"

	# Now copy all glest binaries
	echo Copying live Mega Glest binary files...

	pushd "`pwd`/$megaglest_linux_path"

	find glest -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find glest.bin -exec cp -p --parents "{}" ${INSTALLDATADIR}bin ';'
	find glest.ini -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find glestkeys.ini -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find glest_configurator -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find glest_editor -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find glest_g3dviewer -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find servers.ini -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find makedeps_folder.sh -exec cp -p --parents "{}" $INSTALL_ROOTDIR ';'

	popd

	# Now copy all glest data
	echo Copying live Mega Glest data files...

	pushd "`pwd`/$megaglest_data_path"

	find configuration.xml -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find megaglest.ico -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find g3dviewer.ico -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find editor.ico -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find megaglest.bmp -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find data/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find docs/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find maps/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find scenarios/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find screens/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find techs/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find tilesets/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'
	find tutorials/ \( -name "*.*" \) -exec cp -p --parents "{}" $INSTALLDATADIR ';'

	popd

	# Now remove svn and temp files
	echo removing temp and svn files...

	find data/ -name "\.svn" -type d -depth -exec rm -rf {} \;
	find data/ -name "*~" -exec rm -rf {} \;

	# Copy shared lib dependencies for glest.bin
	cd data
	copyGlestDeptsCmd="${INSTALL_ROOTDIR}makedeps_folder.sh bin/glest.bin"
	$copyGlestDeptsCmd
	cd ..
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
cd ../
rm -rf cmake-build
mkdir cmake-build
cd cmake-build
cmake \
    -DCMAKE_BUILD_TYPE=$BUILDTYPE \
    -DCMAKE_C_COMPILER=$CC \
    -DCMAKE_CXX_COMPILER=$CXX \
    -DMOJOSETUP_MULTIARCH=FALSE \
    -DMOJOSETUP_ARCHIVE_ZIP=TRUE \
    -DMOJOSETUP_ARCHIVE_TAR=TRUE \
    -DMOJOSETUP_ARCHIVE_TAR_BZ2=TRUE \
    -DMOJOSETUP_ARCHIVE_TAR_GZ=TRUE \
    -DMOJOSETUP_GUI_GTKPLUS2=TRUE \
    -DMOJOSETUP_GUI_GTKPLUS2_STATIC=TRUE \
    -DMOJOSETUP_GUI_NCURSES=TRUE \
    -DMOJOSETUP_GUI_NCURSES_STATIC=TRUE \
    -DMOJOSETUP_GUI_STDIO=TRUE \
    -DMOJOSETUP_GUI_STDIO_STATIC=TRUE \
    -DMOJOSETUP_GUI_WWW=FALSE \
    -DMOJOSETUP_GUI_WWW_STATIC=FALSE \
    -DMOJOSETUP_LUALIB_DB=FALSE \
    -DMOJOSETUP_LUALIB_IO=FALSE \
    -DMOJOSETUP_LUALIB_MATH=FALSE \
    -DMOJOSETUP_LUALIB_OS=FALSE \
    -DMOJOSETUP_LUALIB_PACKAGE=FALSE \
    -DMOJOSETUP_LUA_PARSER=TRUE \
    -DMOJOSETUP_IMAGE_BMP=TRUE \
    -DMOJOSETUP_IMAGE_JPG=FALSE \
    -DMOJOSETUP_IMAGE_PNG=FALSE \
    -DMOJOSETUP_INTERNAL_BZLIB=TRUE \
    -DMOJOSETUP_INTERNAL_ZLIB=TRUE \
    -DMOJOSETUP_URL_HTTP=FALSE \
    -DMOJOSETUP_URL_FTP=FALSE \
    ..

# Perhaps needed to remove compiler / linker warnings considered as errors
# sed -i 's/-Werror//' Makefile

make -j$NCPU

# Strip the binaries and GUI plugins, put them somewhere useful.
if [ "$DEBUG" != "1" ]; then
    strip ./mojosetup
fi

mv ./mojosetup ../megaglest-installer/megaglest-installer
for feh in *.so *.dll *.dylib ; do
    if [ -f $feh ]; then
        if [ "$DEBUG" != "1" ]; then
            strip $feh
        fi
        mv $feh ../megaglest-installer/image/guis
    fi
done

# Compile the Lua scripts, put them in the base archive.
for feh in ../scripts/*.lua ; do
    ./mojoluac $LUASTRIPOPT -o ../megaglest-installer/image/scripts/${feh}c $feh
done

# Don't want the example config...use our's instead.
rm -f ../megaglest-installer/image/scripts/config.luac
./mojoluac $LUASTRIPOPT -o ../megaglest-installer/image/scripts/config.luac ../megaglest-installer/scripts/config.lua

# Don't want the example app_localization...use our's instead.
rm -f ../megaglest-installer/image/scripts/app_localization.luac
./mojoluac $LUASTRIPOPT -o ../megaglest-installer/image/scripts/app_localization.luac ../megaglest-installer/scripts/app_localization.lua

# Fill in the rest of the Base Archive...
cd ../megaglest-installer

cp -R data/* image/data/

# remove svn files
echo removing temp and svn files
find image/data/ -name "\.svn" -type d -depth -exec rm -rf {} \;
find image/data/ -name "*~" -exec rm -rf {} \;
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
    cp ../meta/xdg-utils/* image/meta/xdg-utils/
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

# create the compressed image for the installer
#    zip -9r ../${megaglest_archivefilename} *
    ${megaglest_archiver_app} ../${megaglest_archivefilename} *

    cd ..
    rm -rf image
    # Append the archive to the mojosetup binary, so it's "self-extracting."
    cat ${megaglest_archivefilename} >> ./megaglest-installer
    rm -f ${megaglest_archivefilename}
fi

# ...and that's that.
set +e
set +x
echo "Successfully built!"

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

