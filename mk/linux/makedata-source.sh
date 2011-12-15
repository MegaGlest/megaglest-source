#!/bin/bash
# Use this script to build MegaGlest Data Source Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data-source
#PACKAGE="$RELEASENAME-$VERSION.7z"
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"

echo "Creating data package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

project_parent_dir="$(basename $(readlink -f -- "$(dirname -- "$0")/../../") )"
# echo "$project_parent_dir"
if [[ $project_parent_dir == trunk* ]] ;
then
    	echo 'this is the trunk!'
else
	echo '*NOTE: This script currently only works on the trunk, aborting!'
	exit
fi

# copy data
svn export --force "$CURRENTDIR/../../../trunk-data-source" "$RELEASEDIR/data-source/"

echo "creating $PACKAGE"
[[ -f "$release/$PACKAGE" ]] && rm "release/$PACKAGE"
#tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/" "$RELEASENAME-$VERSION"
tar -cf - -C "$CURRENTDIR/release/$RELEASENAME-$VERSION/" "megaglest-$VERSION" | xz -9e > release/$PACKAGE
# 7z a -mmt -mx=9 -ms=on -mhc=on "release/$PACKAGE" "$CURRENTDIR/release/$RELEASENAME-$VERSION"

ls -la release/$PACKAGE
