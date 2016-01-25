#!/bin/bash
# Use this script to build MegaGlest Data Archive for a Version Release
# (Data archive for 'make install', without embedded content)
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
RELEASEDIR="$RELEASEDIR_ROOT/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"
REPODIR="$CURRENTDIR/../../"

echo "Creating data package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

# copy data
echo "Exporting image and ini files ..."
cd "$RELEASEDIR"
git archive --remote ${REPODIR}/data/glest_game/ HEAD: CMakeLists.txt | tar x

echo "Exporting game data files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/data/"
cd "$RELEASEDIR/data/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:data | tar x

echo "Exporting doc files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/docs/"
cd "$RELEASEDIR/docs/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:docs | tar x

cd "$RELEASEDIR/docs/"
git archive --remote ${REPODIR} HEAD:docs/ CHANGELOG.txt | tar x

cd "$RELEASEDIR/docs/"
git archive --remote ${REPODIR} HEAD:docs/ README.txt | tar x

echo "Exporting map files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/maps/"
cd "$RELEASEDIR/maps/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:maps | tar x

echo "Exporting scenario files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/scenarios/"
cd "$RELEASEDIR/scenarios/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:scenarios | tar x

echo "Exporting tech files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/techs/"
cd "$RELEASEDIR/techs/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:techs | tar x

echo "Exporting tileset files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/tilesets/"
cd "$RELEASEDIR/tilesets/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:tilesets | tar x

echo "Exporting tutorial files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/tutorials/"
cd "$RELEASEDIR/tutorials/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:tutorials | tar x

echo "Exporting files from 'others' directory ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/others/"
cd "$RELEASEDIR/others/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:others | tar x

echo "Removing non required files ..."
cd "$CURRENTDIR"
# START
# remove embedded data
rm -rf "$RELEASEDIR/data/core/fonts"
rm -rf "$RELEASEDIR/data/core/misc_textures/flags"
# END

echo "creating $PACKAGE"
[[ -f "$RELEASEDIR_ROOT/$PACKAGE" ]] && rm "$RELEASEDIR_ROOT/$PACKAGE"
tar -cf - -C "$RELEASEDIR_ROOT/$RELEASENAME-$VERSION/" "megaglest-$VERSION" | xz > $RELEASEDIR_ROOT/$PACKAGE

ls -la $RELEASEDIR_ROOT/$PACKAGE

