#!/bin/bash
# Use this script to build MegaGlest Embedded Library Source Code Archive for 
# a Version Release
# (Archive for 'make install', with embedded/missing content in other archives)
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

VERSION=$(./mg-version.sh --version)
RELEASENAME=megaglest-source-embedded
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
RELEASEDIR="$RELEASEDIR_ROOT/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"
REPODIR="$CURRENTDIR/../../"

echo "Creating source package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

mkdir -p "$RELEASEDIR/source/shared_lib/sources/libircclient/"
mkdir -p "$RELEASEDIR/source/shared_lib/include/libircclient/"
cd "$RELEASEDIR/source/shared_lib/sources/libircclient/"
git archive --remote ${REPODIR} HEAD:source/shared_lib/sources/libircclient | tar x
cd "$RELEASEDIR/source/shared_lib/include/libircclient/"
git archive --remote ${REPODIR} HEAD:source/shared_lib/include/libircclient | tar x

mkdir -p "$RELEASEDIR/source/shared_lib/sources/platform/miniupnpc/"
mkdir -p "$RELEASEDIR/source/shared_lib/include/platform/miniupnpc/"
cd "$RELEASEDIR/source/shared_lib/sources/platform/miniupnpc/"
git archive --remote ${REPODIR} HEAD:source/shared_lib/sources/platform/miniupnpc | tar x
cd "$RELEASEDIR/source/shared_lib/include/platform/miniupnpc/"
git archive --remote ${REPODIR} HEAD:source/shared_lib/include/platform/miniupnpc | tar x

mkdir -p "$RELEASEDIR/data/core/misc_textures/flags/"
cd "$RELEASEDIR/data/core/misc_textures/flags/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:data/core/misc_textures/flags | tar x

mkdir -p "$RELEASEDIR/data/core/fonts/"
cd "$RELEASEDIR/data/core/fonts/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:data/core/fonts | tar x

cd "$CURRENTDIR"
echo "Creating $PACKAGE"
[[ -f "$RELEASEDIR_ROOT/$PACKAGE" ]] && rm "$RELEASEDIR_ROOT/$PACKAGE"
tar cJf "$RELEASEDIR_ROOT/$PACKAGE" -C "$RELEASEDIR_ROOT/$RELEASENAME-$VERSION" "megaglest-$VERSION"

ls -la $RELEASEDIR_ROOT/$PACKAGE

