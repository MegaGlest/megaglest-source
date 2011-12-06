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

mkdir -p "$RELEASEDIR/mk/cmake/"
svn export --force "$CURRENTDIR/../cmake/" "$RELEASEDIR/mk/cmake/"
mkdir -p "$RELEASEDIR/mk/macosx/"
svn export --force "$CURRENTDIR/../macosx/" "$RELEASEDIR/mk/macosx/"
mkdir -p "$RELEASEDIR/mk/windoze/"
svn export --force "$CURRENTDIR/../windoze/" "$RELEASEDIR/mk/windoze/"

svn export --force "$CURRENTDIR/../../docs" $RELEASEDIR

#cp "$CURRENTDIR/"*.ini $RELEASEDIR
svn export --force "$CURRENTDIR/glest.ini" $RELEASEDIR/glest.ini
svn export --force "$CURRENTDIR/glestkeys.ini" $RELEASEDIR/glestkeys.ini
svn export --force "$CURRENTDIR/servers.ini" $RELEASEDIR/servers.ini

#cp "$CURRENTDIR/glest.ico" $RELEASEDIR
svn export --force "$CURRENTDIR/glest.ico" $RELEASEDIR/glest.ico

#cp "$CURRENTDIR/megaglest.bmp" $RELEASEDIR
svn export --force "$CURRENTDIR/megaglest.bmp" $RELEASEDIR/megaglest.bmp

#cp "$CURRENTDIR/megaglest.png" $RELEASEDIR
svn export --force "$CURRENTDIR/megaglest.png" $RELEASEDIR/megaglest.png

#cp "$CURRENTDIR/megaglest.desktop" $RELEASEDIR
svn export --force "$CURRENTDIR/megaglest.desktop" $RELEASEDIR/megaglest.desktop

#cp "$CURRENTDIR/megaglest.6" $RELEASEDIR
svn export --force "$CURRENTDIR/megaglest.6" $RELEASEDIR/megaglest.6

#cp "$CURRENTDIR/../../data/glest_game/megaglest.ico" $RELEASEDIR
svn export --force "$CURRENTDIR/../../data/glest_game/megaglest.ico" $RELEASEDIR/megaglest.ico

#cp "$CURRENTDIR/../../data/glest_game/g3dviewer.ico" $RELEASEDIR
svn export --force "$CURRENTDIR/../../data/glest_game/g3dviewer.ico" $RELEASEDIR/g3dviewer.ico

#cp "$CURRENTDIR/../../data/glest_game/editor.ico" $RELEASEDIR
svn export --force "$CURRENTDIR/../../data/glest_game/editor.ico" $RELEASEDIR/editor.ico

#cp "$CURRENTDIR/start_megaglest"* $RELEASEDIR
svn export --force "$CURRENTDIR/start_megaglest" $RELEASEDIR/start_megaglest
svn export --force "$CURRENTDIR/start_megaglest_configurator" $RELEASEDIR/start_megaglest_configurator
svn export --force "$CURRENTDIR/start_megaglest_g3dviewer" $RELEASEDIR/start_megaglest_g3dviewer
svn export --force "$CURRENTDIR/start_megaglest_mapeditor" $RELEASEDIR/start_megaglest_mapeditor

#cp "$CURRENTDIR/../../CMake"* $RELEASEDIR
svn export --force "$CURRENTDIR/../../CMakeLists.txt" $RELEASEDIR/CMakeLists.txt

echo "Creating $PACKAGE"
[[ -f "$release/$PACKAGE" ]] && rm "release/$PACKAGE"
tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/$RELEASENAME-$VERSION" "megaglest-$VERSION"
