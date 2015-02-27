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

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION="$(../linux/mg-version.sh --version)"
kernel="macos"

RELEASENAME="megaglest-binary-$kernel"
PACKAGE="$RELEASENAME-$VERSION.tar.bz2"
CURRENTDIR="$(cd "$(dirname "$0")"; pwd)"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"

echo "Creating binary package in $RELEASEDIR"

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
cp megaglest "$RELEASEDIR"
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

echo "creating $PACKAGE"
cd "$RELEASEDIR_ROOT"
if [ -f "$PACKAGE" ]; then rm "$PACKAGE"; fi
cd "$RELEASENAME"
tar -cf - * | bzip2 -9 > "../$PACKAGE"
cd "$CURRENTDIR"

ls -lhA "${RELEASEDIR_ROOT}/$PACKAGE"
