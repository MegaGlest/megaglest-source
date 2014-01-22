#!/bin/bash
# Use this script to build MegaGlest Data Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION=`./mg-version.sh --version`
RELEASENAME=megaglest-data
#PACKAGE="$RELEASENAME-$VERSION.7z"
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR="$CURRENTDIR/release/$RELEASENAME-$VERSION/megaglest-$VERSION"
SOURCEDIR="$CURRENTDIR/../../source/"
REPODIR="$CURRENTDIR/../../"

echo "Creating data package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

# copy data
echo "Exporting image and ini files ..."
cd "$RELEASEDIR"
# svn export --force "$CURRENTDIR/../../mk/linux/megaglest.bmp" "$RELEASEDIR/megaglest.bmp"
# svn export --force "$CURRENTDIR/../../mk/linux/megaglest.desktop" "$RELEASEDIR/megaglest.desktop"
# svn export --force "$CURRENTDIR/../../mk/linux/megaglest.png" "$RELEASEDIR/megaglest.png"
# svn export --force "$CURRENTDIR/../../mk/linux/megaglest.xpm" "$RELEASEDIR/megaglest.xpm"
# svn export --force "$CURRENTDIR/../../mk/linux/glest.ico" "$RELEASEDIR/glest.ico"
# svn export --force "$CURRENTDIR/../../mk/linux/configuration.xml" "$RELEASEDIR/configuration.xml"

# svn export --force "$CURRENTDIR/../../data/glest_game/megaglest.ico" "$RELEASEDIR/megaglest.ico"
# svn export --force "$CURRENTDIR/../../data/glest_game/g3dviewer.ico" "$RELEASEDIR/g3dviewer.ico"
# svn export --force "$CURRENTDIR/../../data/glest_game/editor.ico" "$RELEASEDIR/editor.ico"
# svn export --force "$CURRENTDIR/../../data/glest_game/servers.ini" "$RELEASEDIR/servers.ini"
# svn export --force "$CURRENTDIR/../../data/glest_game/glest.ini" "$RELEASEDIR/glest_windows.ini"
# svn export --force "$CURRENTDIR/../../mk/linux/glest.ini" "$RELEASEDIR/glest_linux.ini"
# svn export --force "$CURRENTDIR/../../data/glest_game/glestkeys.ini" "$RELEASEDIR/glestkeys.ini"
git archive --remote ${REPODIR}/data/glest_game/ HEAD: CMakeLists.txt | tar x
#git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.bmp | tar x
#git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.desktop | tar x
#git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.png | tar x
#git archive --remote ${REPODIR} HEAD:mk/linux/ megaglest.xpm | tar x
#git archive --remote ${REPODIR} HEAD:mk/linux/ glest.ico | tar x
#git archive --remote ${REPODIR} HEAD:mk/linux/ glest.ini | tar x
#mv glest.ini glest_linux.ini
#git archive --remote ${REPODIR}/data/glest_game/ HEAD: megaglest.ico | tar x
#git archive --remote ${REPODIR}/data/glest_game/ HEAD: g3dviewer.ico | tar x
#git archive --remote ${REPODIR}/data/glest_game/ HEAD: editor.ico | tar x
#git archive --remote ${REPODIR}/data/glest_game/ HEAD: servers.ini | tar x
#git archive --remote ${REPODIR}/data/glest_game/ HEAD: glest.ini | tar x
#git archive --remote ${REPODIR}/data/glest_game/ HEAD: glestkeys.ini | tar x
#mv glest.ini glest_windows.ini

echo "Exporting game data files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/data/"
cd "$RELEASEDIR/data/"
# svn export --force "$CURRENTDIR/../../data/glest_game/data" "$RELEASEDIR/data/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:data | tar x

echo "Exporting doc files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/docs/"
cd "$RELEASEDIR/docs/"
#svn export --force "$CURRENTDIR/../../data/glest_game/docs" "$RELEASEDIR/docs/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:docs | tar x

cd "$RELEASEDIR/docs/"
#svn export --force "$CURRENTDIR/../../docs/CHANGELOG.txt" "$RELEASEDIR/docs/CHANGELOG.txt"
git archive --remote ${REPODIR} HEAD:docs/ CHANGELOG.txt | tar x

cd "$RELEASEDIR/docs/"
#svn export --force "$CURRENTDIR/../../docs/README.txt" "$RELEASEDIR/docs/README.txt"
git archive --remote ${REPODIR} HEAD:docs/ README.txt | tar x

echo "Exporting map files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/maps/"
cd "$RELEASEDIR/maps/"
#svn export --force "$CURRENTDIR/../../data/glest_game/maps" "$RELEASEDIR/maps/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:maps | tar x

echo "Exporting scenario files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/scenarios/"
cd "$RELEASEDIR/scenarios/"
#svn export --force "$CURRENTDIR/../../data/glest_game/scenarios" "$RELEASEDIR/scenarios/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:scenarios | tar x

echo "Exporting tech files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/techs/"
cd "$RELEASEDIR/techs/"
#svn export --force "$CURRENTDIR/../../data/glest_game/techs" "$RELEASEDIR/techs/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:techs | tar x

echo "Exporting tileset files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/tilesets/"
cd "$RELEASEDIR/tilesets/"
#svn export --force "$CURRENTDIR/../../data/glest_game/tilesets" "$RELEASEDIR/tilesets/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:tilesets | tar x

echo "Exporting tutorial files ..."
cd "$RELEASEDIR"
mkdir -p "$RELEASEDIR/tutorials/"
cd "$RELEASEDIR/tutorials/"
# svn export --force "$CURRENTDIR/../../data/glest_game/tutorials" "$RELEASEDIR/tutorials/"
git archive --remote ${REPODIR}/data/glest_game/ HEAD:tutorials | tar x

# special export for flag images
# mkdir -p "$RELEASEDIR/data/core/misc_textures/flags/"
# svn export --force "$CURRENTDIR/../../source/masterserver/flags" "$RELEASEDIR/data/core/misc_textures/flags/"

# cd "$RELEASEDIR"
#svn export --force "$CURRENTDIR/../../data/glest_game/CMakeLists.txt" "$RELEASEDIR/CMakeLists.txt"
# git archive --remote ${REPODIR}/data/glest_game/ HEAD: CMakeLists.txt | tar x

echo "Removing non required files ..."
cd "$CURRENTDIR"
# START
# remove embedded data
rm -rf "$RELEASEDIR/data/core/fonts"
# END

echo "creating $PACKAGE"
[[ -f "$release/$PACKAGE" ]] && rm "release/$PACKAGE"
#tar cJf "release/$PACKAGE" -C "$CURRENTDIR/release/" "$RELEASENAME-$VERSION"
tar -cf - -C "$CURRENTDIR/release/$RELEASENAME-$VERSION/" "megaglest-$VERSION" | xz > release/$PACKAGE
# 7z a -mmt -mx=9 -ms=on -mhc=on "release/$PACKAGE" "$CURRENTDIR/release/$RELEASENAME-$VERSION"

ls -la release/$PACKAGE
