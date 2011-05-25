#!/bin/bash

VERSION=$(./mg-version.sh --version)
RELEASENAME=megaglest-source
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"

echo "Creating source package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

svn export --force "$SOURCEDIR" "$RELEASEDIR/source"

#cp -r "$CURRENTDIR/../cmake/" "$RELEASEDIR"
#cp -r "$CURRENTDIR/../macosx/" "$RELEASEDIR"
#cp -r "$CURRENTDIR/../../docs" $RELEASEDIR
#cp "$CURRENTDIR/"*.ini $RELEASEDIR
#cp "$CURRENTDIR/glest.ico" $RELEASEDIR
#cp "$CURRENTDIR/megaglest.png" $RELEASEDIR
#cp "$CURRENTDIR/megaglest.desktop" $RELEASEDIR
#cp "$CURRENTDIR/start_megaglest"* $RELEASEDIR
#cp "$CURRENTDIR/../../CMake"* $RELEASEDIR
mkdir -p "$RELEASEDIR/mk/cmake/"
svn export --force "$CURRENTDIR/../cmake/" "$RELEASEDIR/mk/cmake/"
mkdir -p "$RELEASEDIR/mk/macosx/"
svn export --force "$CURRENTDIR/../macosx/" "$RELEASEDIR/mk/macosx/"
svn export --force "$CURRENTDIR/../../docs" $RELEASEDIR
cp "$CURRENTDIR/"*.ini $RELEASEDIR
cp "$CURRENTDIR/glest.ico" $RELEASEDIR
cp "$CURRENTDIR/megaglest.png" $RELEASEDIR
cp "$CURRENTDIR/megaglest.desktop" $RELEASEDIR
cp "$CURRENTDIR/start_megaglest"* $RELEASEDIR
cp "$CURRENTDIR/../../CMake"* $RELEASEDIR

echo "Creating $PACKAGE"
[[ -f "$release/$PACKAGE" ]] && rm "release/$PACKAGE"
tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/$RELEASENAME-$VERSION" "megaglest-$VERSION"
