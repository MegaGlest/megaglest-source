#!/bin/bash

VERSION=`autoconf -t AC_INIT | sed -e 's/[^:]*:[^:]*:[^:]*:[^:]*:\([^:]*\):.*/\1/g'`
RELEASENAME=megaglest-data
RELEASEDIR="`pwd`/release/$RELEASENAME-$VERSION"

echo "Creating data package in $RELEASEDIR"

rm -rf $RELEASEDIR
mkdir -p $RELEASEDIR
# copy sources
pushd "`pwd`/../../data/glest_game"
find data/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find docs/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find maps/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find scenarios/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find screenshots/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find techs/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tilesets/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
find tutorials/ \( -name "*.*" \) -exec cp -p --parents "{}" $RELEASEDIR ';'
popd

pushd $RELEASEDIR
find -name "\.svn" -exec rm -rf {} \;
popd

pushd release
PACKAGE="$RELEASENAME-$VERSION.tar.bz2"
echo "creating $PACKAGE"

pushd $RELEASEDIR
tar -c --bzip2 -f "../$PACKAGE" *
popd
