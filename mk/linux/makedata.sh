#!/bin/bash

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data
RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION"

echo "Creating data package in $RELEASEDIR"

rm -rf $RELEASEDIR
mkdir -p $RELEASEDIR
# copy sources
pushd "`pwd`/../../data/glest_game"
find megaglest.ico \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find g3dviewer.ico \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find editor.ico \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find megaglest.bmp \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find servers.ini \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find glestkeys.ini \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find data/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find docs/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find maps/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find scenarios/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find techs/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tilesets/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tutorials/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
popd

pushd $RELEASEDIR

# remove svn files
find -name "\.svn" -type d -depth -exec rm -rf {} \;

popd

pushd release
PACKAGE="$RELEASENAME-$VERSION.7z"
echo "creating $PACKAGE"

pushd $RELEASEDIR
7za a -mx=9 -ms=on -mhc=on "../$PACKAGE" *
popd
