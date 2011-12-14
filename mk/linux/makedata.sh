#!/bin/bash
# Use this script to build MegaGlest Data Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data
#PACKAGE="$RELEASENAME-$VERSION.7z"
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"

echo "Creating data package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

# copy data
svn export --force "$CURRENTDIR/../../mk/linux/megaglest.bmp" "$RELEASEDIR/megaglest.bmp"
svn export --force "$CURRENTDIR/../../mk/linux/megaglest.desktop" "$RELEASEDIR/megaglest.desktop"
svn export --force "$CURRENTDIR/../../mk/linux/megaglest.png" "$RELEASEDIR/megaglest.png"
svn export --force "$CURRENTDIR/../../mk/linux/megaglest.xpm" "$RELEASEDIR/megaglest.xpm"
svn export --force "$CURRENTDIR/../../mk/linux/glest.ico" "$RELEASEDIR/glest.ico"
svn export --force "$CURRENTDIR/../../mk/linux/configuration.xml" "$RELEASEDIR/configuration.xml"
svn export --force "$CURRENTDIR/../../data/glest_game/megaglest.ico" "$RELEASEDIR/megaglest.ico"
svn export --force "$CURRENTDIR/../../data/glest_game/g3dviewer.ico" "$RELEASEDIR/g3dviewer.ico"
svn export --force "$CURRENTDIR/../../data/glest_game/editor.ico" "$RELEASEDIR/editor.ico"
svn export --force "$CURRENTDIR/../../data/glest_game/servers.ini" "$RELEASEDIR/servers.ini"
svn export --force "$CURRENTDIR/../../data/glest_game/glest.ini" "$RELEASEDIR/glest_windows.ini"
svn export --force "$CURRENTDIR/../../mk/linux/glest.ini" "$RELEASEDIR/glest_linux.ini"
svn export --force "$CURRENTDIR/../../data/glest_game/glestkeys.ini" "$RELEASEDIR/glestkeys.ini"

mkdir -p "$RELEASEDIR/data/"
svn export --force "$CURRENTDIR/../../data/glest_game/data" "$RELEASEDIR/data/"
mkdir -p "$RELEASEDIR/docs/"
svn export --force "$CURRENTDIR/../../data/glest_game/docs" "$RELEASEDIR/docs/"
mkdir -p "$RELEASEDIR/maps/"
svn export --force "$CURRENTDIR/../../data/glest_game/maps" "$RELEASEDIR/maps/"
mkdir -p "$RELEASEDIR/scenarios/"
svn export --force "$CURRENTDIR/../../data/glest_game/scenarios" "$RELEASEDIR/scenarios/"
mkdir -p "$RELEASEDIR/techs/"
svn export --force "$CURRENTDIR/../../data/glest_game/techs" "$RELEASEDIR/techs/"
mkdir -p "$RELEASEDIR/tilesets/"
svn export --force "$CURRENTDIR/../../data/glest_game/tilesets" "$RELEASEDIR/tilesets/"
mkdir -p "$RELEASEDIR/tutorials/"
svn export --force "$CURRENTDIR/../../data/glest_game/tutorials" "$RELEASEDIR/tutorials/"

# special export for flag images
mkdir -p "$RELEASEDIR/data/core/misc_textures/flags/"
svn export --force "$CURRENTDIR/../../source/masterserver/flags" "$RELEASEDIR/data/core/misc_textures/flags/"

svn export --force "$CURRENTDIR/../../data/glest_game/CMakeLists.txt" "$RELEASEDIR/CMakeLists.txt"

echo "creating $PACKAGE"
[[ -f "$release/$PACKAGE" ]] && rm "release/$PACKAGE"
#tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/" "$RELEASENAME-$VERSION"
tar -cf - -C "$CURRENTDIR/release/$RELEASENAME-$VERSION/" "megaglest-$VERSION" | xz -9e > release/$PACKAGE
# 7z a -mmt -mx=9 -ms=on -mhc=on "release/$PACKAGE" "$CURRENTDIR/release/$RELEASENAME-$VERSION"

