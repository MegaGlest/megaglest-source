#!/bin/bash

VERSION=`autoconf -t AC_INIT | sed -e 's/[^:]*:[^:]*:[^:]*:[^:]*:\([^:]*\):.*/\1/g'`
RELEASENAME=megaglest-data
RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION"

echo "Creating data package in $RELEASEDIR"

rm -rf $RELEASEDIR
mkdir -p $RELEASEDIR
# copy sources
pushd "`pwd`/../../data/glest_game"
find megaglest.ico \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find g3dviewer.ico \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find editor.ico \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find megaglest.bmp \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find servers.ini \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find glestkeys.ini \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find data/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find docs/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find maps/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find scenarios/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find screens/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find techs/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tilesets/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tutorials/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
popd

pushd $RELEASEDIR

# remove svn files
find -name "\.svn" -type d -depth -exec rm -rf {} \;

popd

pushd release
PACKAGE="$RELEASENAME-$VERSION.7z"
echo "creating $PACKAGE"

pushd $RELEASEDIR
7za a "../$PACKAGE" *
popd
