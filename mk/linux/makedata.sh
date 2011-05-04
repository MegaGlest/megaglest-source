#!/bin/bash

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data
#RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION"
RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION/megaglest-$VERSION"

echo "Creating data package in $RELEASEDIR"

rm -rf $RELEASEDIR
mkdir -p $RELEASEDIR

# copy data
pushd "`pwd`/../../mk/linux"
echo '----In mk/linux'
find megaglest.bmp \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find megaglest.desktop \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find megaglest.png \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
popd
#ls $RELEASEDIR

# copy data
pushd "`pwd`/../../data/glest_game"
echo '----In data/glest_game'

find megaglest.ico \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find g3dviewer.ico \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find editor.ico \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find servers.ini \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find glest.ini \( -name "*" \) -exec cp -p "{}" $RELEASEDIR/glest_linux.ini ';'
find glestkeys.ini \( -name "*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'

find data/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find docs/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find maps/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find scenarios/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find techs/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tilesets/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tutorials/ \( -name "*" \) -not \( -name .svn -prune \) -not \( -name "*~" -prune \) -not \( -name "*.bak" -prune \) -exec cp -p --parents "{}" $RELEASEDIR ';'

cp -p ../../CMake* $RELEASEDIR
popd

pushd $RELEASEDIR

# remove svn files
find -name "\.svn" -type d -depth -exec rm -rf {} \;

popd

pushd release
PACKAGE="$RELEASENAME-$VERSION.7z"
echo "creating $PACKAGE"
rm "$PACKAGE"

#pushd $RELEASEDIR
7za a -mx=9 -ms=on -mhc=on "$PACKAGE" "$RELEASENAME-$VERSION"

popd
