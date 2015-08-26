#!/bin/bash
# Use this script to build MegaGlest Binary Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

# set this to non 0 to skip building the binary
skipbinarybuild=0

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

VERSION=`./mg-version.sh --version`
kernel=`uname -s | tr '[A-Z]' '[a-z]'`
architecture=`uname -m  | tr '[A-Z]' '[a-z]'`

RELEASENAME=megaglest-binary-$kernel-$architecture
PACKAGE="$RELEASENAME-$VERSION.tar.xz"
CURRENTDIR="$(dirname $(readlink -f $0))"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release/"
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"
PROJDIR="$CURRENTDIR/"
REPODIR="$CURRENTDIR/../../"
if [ -d "$REPODIR/.git" ] && [ "$(which git 2>/dev/null)" != "" ]; then
    cd "$REPODIR"
    SOURCE_BRANCH="$(git branch | awk -F '* ' '/^* / {print $2}')"
    SOURCE_COMMIT="$(echo "[$(git rev-list HEAD --count).$(git log -1 --format=%h)]")"
    echo "Detected parameters for source repository: branch=[$SOURCE_BRANCH], commit=$SOURCE_COMMIT"
fi

echo "Creating binary package in $RELEASEDIR"

[[ -d "$RELEASEDIR" ]] && rm -rf "$RELEASEDIR"
mkdir -p "$RELEASEDIR"

if [ $skipbinarybuild = 0 ] 
then
  # build the binaries
  echo "building binaries ..."
  cd $PROJDIR
  [[ -d "build" ]] && rm -rf "build"
  ./build-mg.sh
  if [ $? -ne 0 ]; then
    echo 'ERROR: "./build-mg.sh" failed.' >&2; exit 1
  fi
else
  echo "SKIPPING build of binaries ..."
fi

cd $PROJDIR
mkdir -p "$RELEASEDIR/lib"

[[ -d "lib" ]] && rm -rf "lib"
echo "building binary dependencies ..."
./makedeps_folder.sh megaglest
if [ $? -ne 0 ]; then
  echo 'ERROR: "./makedeps_folder.sh megaglest" failed.' >&2; exit 2
fi

# copy binary info
cd $PROJDIR
echo "copying binaries ..."
cp -r lib/* "$RELEASEDIR/lib"
cp {../shared/,}*.ico {../shared/,}*.ini "$RELEASEDIR/"
if [ -e "$RELEASEDIR/glest-dev.ini" ]; then rm -f "$RELEASEDIR/glest-dev.ini"; fi
cp *.bmp *.png *.xpm *.desktop "$RELEASEDIR/"
cp megaglest megaglest_editor megaglest_g3dviewer start_megaglest \
	start_megaglest_mapeditor start_megaglest_g3dviewer \
	start_megaglest_gameserver "$RELEASEDIR/"

cd "$CURRENTDIR/tools-for-standalone-client"
cp megaglest-configure-desktop.sh "$RELEASEDIR/"
if [ "$(echo "$VERSION" | grep -v '\-dev')" != "" ]; then
    ./prepare-mini-update.sh --only_script; sleep 0.5s
    cp megaglest-mini-update.sh "$RELEASEDIR/"
    if [ -e "megaglest-mini-update.sh" ]; then rm -f "megaglest-mini-update.sh"; fi

    cd $CURRENTDIR
    if [ -e "megaglest" ]; then
	ldd_log="$(echo "$VERSION - $kernel - $architecture - $(date +%F)")"
	ldd_log="$(echo -e "$ldd_log\n\nmegaglest:\n$(ldd megaglest | awk '{print $1}')")"
	if [ -e "megaglest_editor" ]; then
	    ldd_log="$(echo -e "$ldd_log\n\nmegaglest_editor:\n$(ldd megaglest_editor | awk '{print $1}')")"
	fi
	if [ -e "megaglest_g3dviewer" ]; then
	    ldd_log="$(echo -e "$ldd_log\n\nmegaglest_g3dviewer:\n$(ldd megaglest_g3dviewer | awk '{print $1}')")"
	fi
	echo "$ldd_log" > "$RELEASEDIR/ldd-megaglest.log"
    fi
fi

mkdir -p "$RELEASEDIR/blender/"
cp "$CURRENTDIR/../../source/tools/glexemel/"*.py "$RELEASEDIR/blender/"

echo "creating $PACKAGE"
cd $CURRENTDIR
[[ -f "${RELEASEDIR_ROOT}/$PACKAGE" ]] && rm -f "${RELEASEDIR_ROOT}/$PACKAGE"
cd $RELEASEDIR
tar -cf - * | xz > ../$PACKAGE
cd $CURRENTDIR

ls -la ${RELEASEDIR_ROOT}/$PACKAGE
