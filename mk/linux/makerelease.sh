#!/bin/bash
# Use this script to build MegaGlest Source Code Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

VERSION=$(./mg-version.sh --version)
RELEASENAME=megaglest-source
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release/"
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

echo "Exporting files in mk/linux ..."
git archive --remote ${REPODIR} HEAD:mk/linux/ glest.ini | tar x
git archive --remote ${REPODIR} HEAD:mk/shared/ glestkeys.ini | tar x
git archive --remote ${REPODIR} HEAD:mk/shared/ servers.ini | tar x
git archive --remote ${REPODIR} HEAD:mk/shared/ glest.ico | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.bmp | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.png | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.xpm | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.desktop | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest_editor.desktop | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest_g3dviewer.desktop | tar x

echo "Exporting files in data/glest_game ..."
git archive --remote ${REPODIR} HEAD:mk/shared/ megaglest.ico | tar x
git archive --remote ${REPODIR} HEAD:mk/shared/ g3dviewer.ico | tar x
git archive --remote ${REPODIR} HEAD:mk/shared/ editor.ico | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ start_megaglest | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ start_megaglest_g3dviewer | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ start_megaglest_mapeditor | tar x
git archive --remote ${REPODIR} HEAD:mk/linux/ setupBuildDeps.sh | tar x
git archive --remote ${REPODIR} HEAD: CMakeLists.txt | tar x

# exit 1

# START
echo "Removing non required files ..."
# remove embedded library code as that will be packaged in a seperate archive
rm -rf "$RELEASEDIR/source/shared_lib/sources/libircclient/"
rm -rf "$RELEASEDIR/source/shared_lib/include/libircclient/"
rm -rf "$RELEASEDIR/source/shared_lib/sources/platform/miniupnpc/"
rm -rf "$RELEASEDIR/source/shared_lib/include/platform/miniupnpc/"
rm -rf "$RELEASEDIR/source/masterserver/flags/"
# END

cd ${CURRENTDIR}
echo "Creating $PACKAGE"
[[ -f "$RELEASEDIR_ROOT/$PACKAGE" ]] && rm "$RELEASEDIR_ROOT/$PACKAGE"
tar cJf "$RELEASEDIR_ROOT/$PACKAGE" -C "$RELEASEDIR_ROOT/$RELEASENAME-$VERSION" "megaglest-$VERSION"

ls -la $RELEASEDIR_ROOT/$PACKAGE

