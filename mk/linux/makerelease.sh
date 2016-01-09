#!/bin/bash
# Use this script to build MegaGlest Source Code Archive for a Version Release
# (Source archive for 'make install', without embedded content)
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

VERSION=$(./mg-version.sh --version)
RELEASENAME=megaglest-source
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
RELEASEDIR="$RELEASEDIR_ROOT/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"
REPODIR="$CURRENTDIR/../../"

echo "Creating source package in: ${RELEASEDIR} git REPO is in: ${REPODIR}"
# exit 1

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

echo "Exporting source ..."
cd "$RELEASEDIR"
mkdir source
cd source
git archive --remote ${REPODIR} HEAD:source | tar x
cd "$RELEASEDIR"

echo "Exporting mk ..."
mkdir -p mk
cd mk
git archive --remote ${REPODIR} HEAD:mk | tar x
cd "$RELEASEDIR"

echo "Exporting docs ..."
mkdir docs
cd docs
git archive --remote ${REPODIR} HEAD:docs | tar x
cd "$RELEASEDIR"

git archive --remote ${REPODIR} HEAD: CMakeLists.txt | tar x

# exit 1

# START
echo "Removing non required files ..."
# remove embedded library code as that will be packaged in a seperate archive
rm -rf "$RELEASEDIR/source/shared_lib/sources/libircclient/"
rm -rf "$RELEASEDIR/source/shared_lib/include/libircclient/"
rm -rf "$RELEASEDIR/source/shared_lib/sources/platform/miniupnpc/"
rm -rf "$RELEASEDIR/source/shared_lib/include/platform/miniupnpc/"
# END

cd ${CURRENTDIR}
echo "Creating $PACKAGE"
[[ -f "$RELEASEDIR_ROOT/$PACKAGE" ]] && rm "$RELEASEDIR_ROOT/$PACKAGE"
tar cJf "$RELEASEDIR_ROOT/$PACKAGE" -C "$RELEASEDIR_ROOT/$RELEASENAME-$VERSION" "megaglest-$VERSION"

ls -la $RELEASEDIR_ROOT/$PACKAGE

