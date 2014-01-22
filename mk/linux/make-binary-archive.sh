#!/bin/bash
# Use this script to build MegaGlest Binary Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# set this to non 0 to skip building the binary
skipbinarybuild=0

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION=`./mg-version.sh --version`
kernel=`uname -s | tr '[A-Z]' '[a-z]'`
architecture=`uname -m  | tr '[A-Z]' '[a-z]'`

RELEASENAME=megaglest-binary-$kernel-$architecture
#PACKAGE="$RELEASENAME-$VERSION.7z"
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
#RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION/megaglest-$VERSION"
RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION"
#PROJDIR="$CURRENTDIR/../../"
PROJDIR="$CURRENTDIR/"

echo "Creating binary package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

if [ $skipbinarybuild = 0 ] 
then
  # build the binaries
  echo "building binaries ..."
  cd $PROJDIR
  [[ -d "build" ]] && rm -rf "build"
  ./build-mg.sh
  if [ $? -ne 0 ]; then
    echo 'ERROR: "./build-mg.sh" failed.' >&2; exit 1
  fi
else
  echo "SKIPPING build of binaries ..."
fi

cd $PROJDIR
mkdir -p "$RELEASEDIR/lib"
#cd mk/linux
[[ -d "lib" ]] && rm -rf "lib"
echo "building binary dependencies ..."
./makedeps_folder.sh megaglest
if [ $? -ne 0 ]; then
  echo 'ERROR: "./makedeps_folder.sh megaglest" failed.' >&2; exit 2
fi

# copy binary info
cd $PROJDIR
echo "copying binaries ..."
cp -r lib/* "$RELEASEDIR/lib"
cp *.ico "$RELEASEDIR/"
cp *.bmp "$RELEASEDIR/"
cp *.png "$RELEASEDIR/"
cp *.xpm "$RELEASEDIR/"
cp *.ini "$RELEASEDIR/"
cp megaglest "$RELEASEDIR/"
cp megaglest_editor "$RELEASEDIR/"
cp megaglest_g3dviewer "$RELEASEDIR/"
cp start_megaglest "$RELEASEDIR/"
cp start_megaglest_mapeditor "$RELEASEDIR/"
cp start_megaglest_g3dviewer "$RELEASEDIR/"
cp start_megaglest_gameserver "$RELEASEDIR/"

echo "creating $PACKAGE"
cd $CURRENTDIR
[[ -f "$release/$PACKAGE" ]] && rm "release/$PACKAGE"
#tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/" "$RELEASENAME-$VERSION"
#tar -cf - -C "$CURRENTDIR/release/$RELEASENAME-$VERSION/" "./" | xz > release/$PACKAGE
cd release/$RELEASENAME-$VERSION/
tar -cf - * | xz > ../$PACKAGE
cd $CURRENTDIR
# 7z a -mmt -mx=9 -ms=on -mhc=on "release/$PACKAGE" "$CURRENTDIR/release/$RELEASENAME-$VERSION"

ls -la release/$PACKAGE
