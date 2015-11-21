#!/bin/sh
# Use this script to build MegaGlest Binary Archive for a Version Release
# ----------------------------------------------------------------------------
# 2011 Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# 2015 Rewritten by filux <heross(@@)o2.pl>
# Copyright (c) 2011-2015 under GNU GPL v3.0+
# ----------------------------------------------------------------------------
LANG=C

# set this to non 0 to skip building the binary
skipbinarybuild=0
if [ "$1" = "-CI" ]; then skipbinarybuild=1; fi

CURRENTDIR="$(cd "$(dirname "$0")"; pwd)"
cd "$CURRENTDIR"

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

if [ "$1" = "-CI" ] || [ "$1" = "-" ] || [ "$(echo "$1" | grep '\--show-result-path')" != "" ]; then
    if [ "$2" != "" ]; then SOURCE_BRANCH="$2"; fi
fi

VERSION="$(../linux/mg-version.sh --version)"
kernel="macos"
REPODIR="$CURRENTDIR/../../"
if [ -d "$REPODIR/.git" ] && [ "$(which git 2>/dev/null)" != "" ]; then
    cd "$REPODIR"
    if [ "$SOURCE_BRANCH" = "" ]; then SOURCE_BRANCH="$(git branch | grep '^* ' | awk '{print $2}')"; fi
    # on macos are problems with more advanced using awk ^
    SOURCE_COMMIT="$(echo "[$(git rev-list HEAD --count).$(git log -1 --format=%h)]")"
fi

ARCHIVE_TYPE="tar.bz2"
SNAPSHOTNAME="mg-binary-$kernel"
SN_PACKAGE="$SNAPSHOTNAME-$VERSION-$SOURCE_BRANCH.$ARCHIVE_TYPE"
RELEASENAME="megaglest-binary-$kernel"
PACKAGE="$RELEASENAME-$VERSION.$ARCHIVE_TYPE"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"

if [ "$SOURCE_BRANCH" != "" ] && [ "$SOURCE_BRANCH" != "master" ] && [ "$(echo "$VERSION" | grep '\-dev$')" != "" ]; then
    RELEASENAME="$SNAPSHOTNAME"; PACKAGE="$SN_PACKAGE"
fi
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"
if [ "$1" = "--show-result-path" ]; then echo "${RELEASEDIR_ROOT}/$PACKAGE"; exit 0
elif [ "$1" = "--show-result-path2" ]; then echo "${RELEASEDIR_ROOT}/$RELEASENAME"; exit 0; fi

echo "Creating binary package in $RELEASEDIR"
if [ "$SOURCE_BRANCH" != "" ]; then echo "Detected parameters for source repository: branch=[$SOURCE_BRANCH], commit=$SOURCE_COMMIT"; fi

if [ -d "$RELEASEDIR" ]; then rm -rf "$RELEASEDIR"; fi
mkdir -p "$RELEASEDIR"

if [ "$skipbinarybuild" -eq "0" ]; then
	echo "building binaries ..."
	cd "$CURRENTDIR"
	if [ -d "build" ]; then rm -rf "build"; fi
	./build-mg.sh -b
	if [ "$?" -ne "0" ]; then echo 'ERROR: "./build-mg.sh" failed.' >&2; exit 1; fi
else
	echo "SKIPPING build of binaries ..."
fi

cd "$CURRENTDIR"
echo "copying binaries ..."
cp ../shared/*.ico "$RELEASEDIR"
if [ -e "$RELEASEDIR/glest.ico" ]; then rm "$RELEASEDIR/glest.ico"; fi
#cp bundle_resources/*.icns "$RELEASEDIR"
cp {../shared/,}*.ini "$RELEASEDIR"
if [ -e "$RELEASEDIR/glest-dev.ini" ]; then rm "$RELEASEDIR/glest-dev.ini"; fi
cp megaglest ../linux/start_megaglest_gameserver bundle_resources/MegaGlest.sh "$RELEASEDIR"

if [ -e "megaglest_editor" ]; then
	cp megaglest_editor "$RELEASEDIR"
else
	if [ -e "$RELEASEDIR/editor.ico" ]; then rm "$RELEASEDIR/editor.ico"; fi
fi
if [ -e "megaglest_g3dviewer" ]; then
	cp megaglest_g3dviewer "$RELEASEDIR"
else
	if [ -e "$RELEASEDIR/g3dviewer.ico" ]; then rm "$RELEASEDIR/g3dviewer.ico"; fi
fi
if [ -d "p7zip" ]; then cp -r p7zip "$RELEASEDIR"; fi
if [ -d "lib" ]; then cp -r lib "$RELEASEDIR"; fi
if [ "$(echo "$VERSION" | grep -v '\-dev$')" != "" ]; then
    echo "$(date -u)" > "$RELEASEDIR/build-time.log"
fi

if [ "$1" != "--installer" ]; then
    echo "creating $PACKAGE"
    cd "$RELEASEDIR_ROOT"
    if [ -f "$PACKAGE" ]; then rm "$PACKAGE"; fi
    cd "$RELEASENAME"
    tar -cf - * | bzip2 -9 > "../$PACKAGE"
    cd "$CURRENTDIR"
    ls -lhA "${RELEASEDIR_ROOT}/$PACKAGE"
fi

cd "$CURRENTDIR"
if [ "$1" = "-CI" ] || [ "$1" = "-" ]; then
	if [ -d "$RELEASEDIR" ]; then rm -rf "$RELEASEDIR"; fi
fi
