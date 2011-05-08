#!/bin/bash

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
cp "$CURRENTDIR/../../mk/linux/megaglest.bmp" $RELEASEDIR
cp "$CURRENTDIR/../../mk/linux/megaglest.desktop" $RELEASEDIR
cp "$CURRENTDIR/../../mk/linux/megaglest.png" $RELEASEDIR
cp "$CURRENTDIR/../../mk/linux/glest.ico" $RELEASEDIR

cp "$CURRENTDIR/../../data/glest_game/megaglest.ico" $RELEASEDIR
cp "$CURRENTDIR/../../data/glest_game/g3dviewer.ico" $RELEASEDIR
cp "$CURRENTDIR/../../data/glest_game/editor.ico" $RELEASEDIR
cp "$CURRENTDIR/../../data/glest_game/servers.ini" $RELEASEDIR
cp "$CURRENTDIR/../../data/glest_game/glest.ini" $RELEASEDIR
cp "$CURRENTDIR/../../data/glest_game/glestkeys.ini" $RELEASEDIR
cp "$CURRENTDIR/../../data/glest_game/configuration.xml" $RELEASEDIR

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

cp -p "$CURRENTDIR/../../data/glest_game/CMakeLists.txt" $RELEASEDIR

echo "creating $PACKAGE"
rm "release/$PACKAGE"
#tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/" "$RELEASENAME-$VERSION"
tar -cf - -C "$CURRENTDIR/release/$RELEASENAME-$VERSION/" "megaglest-$VERSION" | xz -9e > release/$PACKAGE
# 7z a -mmt -mx=9 -ms=on -mhc=on "release/$PACKAGE" "$CURRENTDIR/release/$RELEASENAME-$VERSION"

