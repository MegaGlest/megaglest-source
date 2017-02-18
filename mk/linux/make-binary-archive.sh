#!/bin/bash
# Use this script to build MegaGlest Binary Archive for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+
LANG=C

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
# set this to non 0 to skip building the binary
skipbinarybuild=0
if [ "$1" = "-CI" ] || ( [ "$1" = "--installer" ] && \
    [ "$(find "$CURRENTDIR" -maxdepth 1 -name 'megaglest' -mmin -60)" ] ); then
    skipbinarybuild=1
fi

# Consider setting this for small packages if there's plenty of RAM and CPU available:
#export XZ_OPT="$XZ_OPT -9e"

if [ "$1" = "-CI" ] || [ "$1" = "--installer" ] || [ "$(echo "$1" | grep '\--show-result-path')" != "" ]; then
    if [ "$2" != "" ]; then SOURCE_BRANCH="$2"; fi
fi

cd "$CURRENTDIR"
VERSION=`./mg-version.sh --version`
kernel=`uname -s | tr '[A-Z]' '[a-z]'`
architecture=`uname -m  | tr '[A-Z]' '[a-z]'`

RELEASEDIR_ROOT="$CURRENTDIR/../../../release/"
PROJDIR="$CURRENTDIR/"
REPODIR="$CURRENTDIR/../../"
REPO_DATADIR="$REPODIR/data/glest_game"
if [ -d "$REPODIR/.git" ] && [ "$(which git 2>/dev/null)" != "" ]; then
    cd "$REPODIR"
    if [ "$SOURCE_BRANCH" = "" ]; then SOURCE_BRANCH="$(git branch | awk -F '* ' '/^* / {print $2}')"; fi
    SOURCE_COMMIT="$(echo "[$(git rev-list HEAD --count).$(git log -1 --format=%h --abbrev=7)]")"
fi
ARCHIVE_TYPE="tar.xz"
SNAPSHOTNAME="mg-binary-$kernel-$architecture"
SN_PACKAGE="$SNAPSHOTNAME-$VERSION-$SOURCE_BRANCH.$ARCHIVE_TYPE"
RELEASENAME="megaglest-binary-$kernel-$architecture"
PACKAGE="$RELEASENAME-$VERSION.$ARCHIVE_TYPE"
if [ "$SOURCE_BRANCH" != "" ] && [ "$SOURCE_BRANCH" != "master" ] && [ "$(echo "$VERSION" | grep '\-dev$')" != "" ]; then
    RELEASENAME="$SNAPSHOTNAME"; PACKAGE="$SN_PACKAGE"
fi
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"
if [ "$1" = "--show-result-path" ]; then echo "${RELEASEDIR_ROOT}/$PACKAGE"; exit 0
elif [ "$1" = "--show-result-path2" ]; then echo "${RELEASEDIR_ROOT}/$RELEASENAME"; exit 0; fi

echo "Creating binary package in $RELEASEDIR"
if [ "$SOURCE_BRANCH" != "" ]; then echo "Detected parameters for source repository: branch=[$SOURCE_BRANCH], commit=$SOURCE_COMMIT"; fi

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

if [ -d "lib" ]; then rm -rf "lib"; fi
echo "building binary dependencies ..."
for mg_bin in megaglest megaglest_editor megaglest_g3dviewer; do
    ./makedeps_folder.sh "$mg_bin"
    if [ "$?" -ne "0" ]; then echo "ERROR: \"./makedeps_folder.sh $mg_bin\" failed." >&2; exit 2; fi
done

# copy binary info
cd $PROJDIR
echo "copying binaries ..."
cp -r lib/* "$RELEASEDIR/lib"
cp ../shared/*.ico {../shared/,}*.ini "$RELEASEDIR/"
if [ -e "$RELEASEDIR/glest-dev.ini" ]; then rm -f "$RELEASEDIR/glest-dev.ini"; fi
cd $REPO_DATADIR/others/icons
cp *.bmp *.png *.xpm "$RELEASEDIR/"
if [ "$1" != "--installer" ]; then cd $REPO_DATADIR/others/desktop; cp *.desktop "$RELEASEDIR/"; fi
cd $PROJDIR
cp megaglest megaglest_editor megaglest_g3dviewer start_megaglest_gameserver "$RELEASEDIR/"

cd "$CURRENTDIR/tools-for-standalone-client"
cp start_megaglest start_megaglest_mapeditor start_megaglest_g3dviewer "$RELEASEDIR/"
if [ "$1" != "--installer" ]; then cp megaglest-configure-desktop.sh "$RELEASEDIR/"; fi

if [ "$(echo "$VERSION" | grep -v '\-dev$')" != "" ]; then
    ./prepare-mini-update.sh --only_script; sleep 0.5s
    cp megaglest-mini-update.sh "$RELEASEDIR/"
    if [ -e "megaglest-mini-update.sh" ]; then rm -f "megaglest-mini-update.sh"; fi

    cd $CURRENTDIR
    if [ -e "megaglest" ] && [ "$1" != "--installer" ]; then
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
    echo "$(date -u)" > "$RELEASEDIR/build-time.log"
fi

mkdir -p "$RELEASEDIR/blender/"
cp "$CURRENTDIR/../../source/tools/glexemel/"*.py "$RELEASEDIR/blender/"

if [ "$1" != "--installer" ]; then
    echo "creating $PACKAGE"
    cd $CURRENTDIR
    [[ -f "${RELEASEDIR_ROOT}/$PACKAGE" ]] && rm -f "${RELEASEDIR_ROOT}/$PACKAGE"
    cd $RELEASEDIR
    tar -cf - * | xz > ../$PACKAGE

    cd $CURRENTDIR
    ls -la ${RELEASEDIR_ROOT}/$PACKAGE
fi
if [ "$1" = "-CI" ] && [ -d "$RELEASEDIR" ]; then rm -rf "$RELEASEDIR"; fi
