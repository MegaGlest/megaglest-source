#!/bin/bash
# Use this script to build MegaGlest Binary Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# set this to non 0 to skip building the binary
skipbinarybuild=0

VERSION=`./mg-version.sh --version`
kernel=`uname -s`
architecture=`uname -m`

RELEASENAME=megaglest-binary-$kernel-$architecture
#PACKAGE="$RELEASENAME-$VERSION.7z"
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
#RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION/megaglest-$VERSION"
RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION"
PROJDIR="$CURRENTDIR/../../"

echo "Creating binary package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

if [ '$skipbinarybuild' = '0' ] 
then

# build the binaries
echo "building binaries ..."
cd $PROJDIR
[[ -d "build" ]] && rm -rf "build"
./build-mg.sh
fi

cd $PROJDIR
mkdir -p "$RELEASEDIR/lib"
cd mk/linux
[[ -d "lib" ]] && rm -rf "lib"
echo "building binary dependencies ..."
./makedeps_folder.sh megaglest

# copy binary info
cd $PROJDIR
echo "copying binaries ..."
cp -r mk/linux/lib/* "$RELEASEDIR/lib"
cp mk/linux/*.ico "$RELEASEDIR/"
cp mk/linux/*.bmp "$RELEASEDIR/"
cp mk/linux/*.png "$RELEASEDIR/"
cp mk/linux/*.xpm "$RELEASEDIR/"
cp mk/linux/*.ini "$RELEASEDIR/"
cp mk/linux/megaglest "$RELEASEDIR/"
cp mk/linux/megaglest_editor "$RELEASEDIR/"
cp mk/linux/megaglest_g3dviewer "$RELEASEDIR/"
cp mk/linux/start_megaglest "$RELEASEDIR/"
cp mk/linux/start_megaglest_mapeditor "$RELEASEDIR/"
cp mk/linux/start_megaglest_g3dviewer "$RELEASEDIR/"

echo "creating $PACKAGE"
cd $CURRENTDIR
[[ -f "$release/$PACKAGE" ]] && rm "release/$PACKAGE"
#tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/" "$RELEASENAME-$VERSION"
#tar -cf - -C "$CURRENTDIR/release/$RELEASENAME-$VERSION/" "./" | xz -9e > release/$PACKAGE
cd release/$RELEASENAME-$VERSION/
tar -cf - * | xz -9e > ../$PACKAGE
cd $CURRENTDIR
# 7z a -mmt -mx=9 -ms=on -mhc=on "release/$PACKAGE" "$CURRENTDIR/release/$RELEASENAME-$VERSION"

ls -la release/$PACKAGE
