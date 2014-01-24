#!/bin/bash
# Use this script to build MegaGlest Data Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-standalone-data
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release/"
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"
PROJDIR="$CURRENTDIR/../../"
REPODIR="$CURRENTDIR/../../"

echo "Creating data package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

# copy data
echo "copying data ..."

mkdir -p "$RELEASEDIR/data/"
cd "$RELEASEDIR/data/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:data | tar x

cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/docs/"
cd "$RELEASEDIR/docs/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:docs | tar x
git archive --remote ${REPODIR} HEAD:docs/ CHANGELOG.txt | tar x
git archive --remote ${REPODIR} HEAD:docs/ README.txt | tar x

cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/maps/"
cd "$RELEASEDIR/maps/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:maps | tar x

cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/scenarios/"
cd "$RELEASEDIR/scenarios/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:scenarios | tar x

cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/techs/"
cd "$RELEASEDIR/techs/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:techs | tar x

cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/tilesets/"
cd "$RELEASEDIR/tilesets/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:tilesets | tar x

cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/tutorials/"
cd "$RELEASEDIR/tutorials/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:tutorials | tar x

# special export for flag images
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/data/core/misc_textures/flags/"
cd "$RELEASEDIR/data/core/misc_textures/flags/"
git archive --remote ${REPODIR} HEAD:source/masterserver/flags | tar x

cd "$CURRENTDIR"
echo "creating data archive: $PACKAGE"
[[ -f "${RELEASEDIR_ROOT}/$PACKAGE" ]] && rm "${RELEASEDIR_ROOT}/$PACKAGE"
cd ${RELEASEDIR_ROOT}/$RELEASENAME-$VERSION
tar -cf - * | xz > ../$PACKAGE
cd $CURRENTDIR

ls -la ${RELEASEDIR_ROOT}/$PACKAGE

