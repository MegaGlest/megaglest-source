#!/bin/bash
# Use this script to build MegaGlest Data Source Archive for a Version Release
# (Archive for 'third' repository)
# ----------------------------------------------------------------------------
# 2011 Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# 2015 Improved by filux <heross(@@)o2.pl>
# Copyright (c) 2011-2015 under GNU GPL v3.0+

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data-source
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
RELEASEDIR="$RELEASEDIR_ROOT/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"
# Below we assume you have the data source contents root in a folder called: megaglest-data-source/
REPODIR="$(readlink -f "$CURRENTDIR/../../../megaglest-data-source/")"
echo "Creating data package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

if [ ! -d "$REPODIR" ]; then
	echo "ERROR: directory not found: $REPODIR"
	exit 1
fi
cd "$REPODIR"
CURRENTCOMMIT="$(git rev-parse HEAD 2>/dev/null)"
if [ "$CURRENTCOMMIT" = "" ] && [ "$(git rev-parse HEAD)" != "" ]; then
	echo "ERROR: git is not installed or git repository not found in: $REPODIR"
	exit 1
fi
WANTEDCOMMIT="$(git show-ref --tags -d | grep "refs/tags/$VERSION" | awk '{print $1}')"
if [ "$WANTEDCOMMIT" != "$CURRENTCOMMIT" ]; then
	echo "*NOTE: This script currently only works on the matching commit to the release's tag, aborting!"
	echo "**Hints: Maybe tag is not set yet, or maybe e.g. you are in wrong branch."
	exit 1
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
