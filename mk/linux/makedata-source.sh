#!/bin/bash
# Use this script to build MegaGlest Data Source Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data-source
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release/"
RELEASEDIR="$RELEASEDIR_ROOT/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"
# Below we assume you have the data source contents root in a folder called: git-data-source/
REPODIR="$CURRENTDIR/../../../git-data-source/"

echo "Creating data package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

project_parent_dir="$(basename $(readlink -f -- "$(dirname -- "$0")/../../") )"
# echo "$project_parent_dir"
if [[ $project_parent_dir == git* ]] ;
then
    	echo 'This is the master branch'
else
	echo '*NOTE: This script currently only works on the master HEAD, aborting!'
	exit
fi


# copy data
mkdir -p "$RELEASEDIR/data-source"
cd "$RELEASEDIR/data-source"
git archive --remote ${REPODIR}/ HEAD: | tar x

cd "$CURRENTDIR"
echo "creating $PACKAGE"
[[ -f "$RELEASEDIR_ROOT/$PACKAGE" ]] && rm "$RELEASEDIR_ROOT/$PACKAGE"
tar -cf - -C "$RELEASEDIR_ROOT/$RELEASENAME-$VERSION/" "megaglest-$VERSION" | xz > $RELEASEDIR_ROOT/$PACKAGE

ls -la $RELEASEDIR_ROOT/$PACKAGE

