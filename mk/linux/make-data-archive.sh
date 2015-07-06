#!/bin/bash
# Use this script to build MegaGlest Data Archive for a Version Release
# (Data archive for 'snapshots', with embedded content)
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

KERNEL="$(uname -s | tr '[A-Z]' '[a-z]')"
if [ "$KERNEL" = "darwin" ]; then
    CURRENTDIR="$(cd "$(dirname "$0")"; pwd)"
else
    CURRENTDIR="$(dirname "$(readlink -f "$0")")"
fi
cd "$CURRENTDIR"
VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-standalone-data
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"
REPODIR="$CURRENTDIR/../../"
REPO_DATADIR="$REPODIR/data/glest_game"
if [ -f "$REPO_DATADIR/.git" ] && [ "$(which git 2>/dev/null)" != "" ]; then
    cd "$REPO_DATADIR"
    DATA_BRANCH="$(git branch | awk -F '* ' '/^* / {print $2}')"
    DATA_COMMIT="$(echo "[$(git rev-list HEAD --count).$(git log -1 --format=%h)]")"
    echo "Detected parameters for data repository: branch=[$DATA_BRANCH], commit=$DATA_COMMIT"
fi
if [ -d "$REPODIR/.git" ] && [ "$(which git 2>/dev/null)" != "" ]; then
    cd "$REPODIR"
    SOURCE_BRANCH="$(git branch | awk -F '* ' '/^* / {print $2}')"
    SOURCE_COMMIT="$(echo "[$(git rev-list HEAD --count).$(git log -1 --format=%h)]")"
    echo "Detected parameters for source repository: branch=[$SOURCE_BRANCH], commit=$SOURCE_COMMIT"
fi
cd "$CURRENTDIR"

if [ "$KERNEL" != "darwin" ]; then
    echo "Creating data package in $RELEASEDIR"
else
    echo "Creating data directory $RELEASEDIR"
fi

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

echo "Removing non required files ..."
cd "$CURRENTDIR"
# START
# END

cd "$CURRENTDIR"
if [ "$KERNEL" != "darwin" ]; then
    echo "creating data archive: $PACKAGE"
    [[ -f "${RELEASEDIR_ROOT}/$PACKAGE" ]] && rm "${RELEASEDIR_ROOT}/$PACKAGE"
    cd $RELEASEDIR
    tar -cf - * | xz > ../$PACKAGE
    cd $CURRENTDIR

    ls -la ${RELEASEDIR_ROOT}/$PACKAGE
fi
