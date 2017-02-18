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
if [ "$1" != "" ] && [ "$2" != "" ]; then SOURCE_BRANCH="$2"; fi
VERSION=`./mg-version.sh --version`

REPODIR="$CURRENTDIR/../../"
REPO_DATADIR="$REPODIR/data/glest_game"
if [ -f "$REPO_DATADIR/.git" ] && [ "$(which git 2>/dev/null)" != "" ]; then
    cd "$REPO_DATADIR"
    DATA_BRANCH="$(git branch | grep '^* ' | awk '{print $2}')"
    # on macos are problems with more advanced using awk ^
    DATA_COMMIT_NR="$(git rev-list HEAD --count)"
    DATA_COMMIT="$(echo "[$DATA_COMMIT_NR.$(git log -1 --format=%h --abbrev=7)]")"
    DATA_HASH=$(git log -1 --format=%H)
fi
if [ -d "$REPODIR/.git" ] && [ "$(which git 2>/dev/null)" != "" ]; then
    cd "$REPODIR"
    if [ "$SOURCE_BRANCH" = "" ]; then SOURCE_BRANCH="$(git branch | grep '^* ' | awk '{print $2}')"; fi
    # on macos are problems with more advanced using awk ^
    SOURCE_COMMIT="$(echo "[$(git rev-list HEAD --count).$(git log -1 --format=%h --abbrev=7)]")"
    if [ "$DATA_HASH" = "" ]; then DATA_HASH=$(git submodule status "$REPO_DATADIR" | awk '{print $1}'); fi
fi
classic_snapshot_for_tests=0
if [ "$SOURCE_BRANCH" != "" ] && [ "$SOURCE_BRANCH" != "master" ] && [ "$(echo "$VERSION" | grep '\-dev$')" != "" ]; then
    classic_snapshot_for_tests=1
fi
if [ "$classic_snapshot_for_tests" -eq "1" ]; then ARCHIVE_TYPE="7z"; else ARCHIVE_TYPE="tar.xz"; fi
SNAPSHOTNAME="mg-data-universal"
SN_PACKAGE="$SNAPSHOTNAME-$VERSION-$SOURCE_BRANCH.$ARCHIVE_TYPE"
RELEASENAME="megaglest-standalone-data"
PACKAGE="$RELEASENAME-$VERSION.$ARCHIVE_TYPE"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
if [ "$classic_snapshot_for_tests" -eq "1" ]; then RELEASENAME="$SNAPSHOTNAME"; PACKAGE="$SN_PACKAGE"; fi
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"
if [ "$1" = "--show-result-path" ]; then echo "${RELEASEDIR_ROOT}/$PACKAGE"; exit 0
elif [ "$1" = "--show-result-path2" ]; then echo "${RELEASEDIR_ROOT}/$RELEASENAME"; exit 0; fi

DATA_HASH_MEMORY="$RELEASEDIR_ROOT/data_memory"
DATA_HASH_FILE="$DATA_HASH_MEMORY/$VERSION-$SOURCE_BRANCH.log"
if [ ! -d "$DATA_HASH_MEMORY" ]; then mkdir -p "$DATA_HASH_MEMORY"; fi
SyncNote() {
echo; echo " This situation is allowed for \"git submodule\", but in MG case it usually mean sync to wrong data commit. In case of wrong sync, to fix the situation someone should again commit in the megaglest-source repo the recent data HASH. If sync to older data wasn't a mistake then just ignore this warning."; echo
}
if [ "$DATA_HASH" != "" ]; then
    if [ -e "$DATA_HASH_FILE" ]; then
	CAT_DATA_HASH_FILE="$(cat "$DATA_HASH_FILE" | head -1)"
	DATA_COMMIT_PREV_NR="$(echo "$CAT_DATA_HASH_FILE" | awk '{print $2}')"
	DATA_COMMIT_LATEST_NR="$(echo "$CAT_DATA_HASH_FILE" | awk '{print $3}')"
    else
	echo "$DATA_HASH $DATA_COMMIT_NR $DATA_COMMIT_NR" > "$DATA_HASH_FILE"
    fi
    if [ -e "$DATA_HASH_FILE" ] && [ "$DATA_COMMIT_PREV_NR" != "" ]; then
	if [ "$DATA_COMMIT_NR" -lt "$DATA_COMMIT_PREV_NR" ]; then
	    echo; echo " warning: Detected older git revision of data than previously, $DATA_COMMIT_NR < $DATA_COMMIT_PREV_NR."
	    SyncNote
	else
	    if [ "$DATA_COMMIT_LATEST_NR" != "" ] && [ "$DATA_COMMIT_NR" -lt "$DATA_COMMIT_LATEST_NR" ]; then
		echo; echo " warning: Detected older git revision of data than synced in the past, $DATA_COMMIT_NR < $DATA_COMMIT_LATEST_NR."
		SyncNote
		if [ "$DATA_COMMIT_NR" -gt "$DATA_COMMIT_PREV_NR" ]; then DATA_COMMIT_LATEST_NR="$DATA_COMMIT_NR"; fi
		# ^ first sync commit -gt than the wrong one = warning seen for last time, so situation still may be not fixed, but is most likely improved and we avoid endless warnings
	    fi
	    if [ "$1" != "--installer" ] && [ "$(echo "$CAT_DATA_HASH_FILE" | grep "$DATA_HASH")" != "" ]; then
		echo; echo " NOTE: The archive wasn't created because for almost sure it would be exactly the same like last time (the same commit) and new creation date would convince many users to download it again."; echo
		exit 0
	    fi
	fi
	if [ "$DATA_COMMIT_LATEST_NR" = "" ] || [ "$DATA_COMMIT_NR" -gt "$DATA_COMMIT_LATEST_NR" ]; then
	    DATA_COMMIT_LATEST_NR="$DATA_COMMIT_NR"
	fi
	echo "$DATA_HASH $DATA_COMMIT_NR $DATA_COMMIT_LATEST_NR" > "$DATA_HASH_FILE"
    fi
fi

cd "$CURRENTDIR"
if [ "$DATA_BRANCH" != "" ]; then echo "Detected parameters for data repository: branch=[$DATA_BRANCH], commit=$DATA_COMMIT"; fi
if [ "$SOURCE_BRANCH" != "" ]; then echo "Detected parameters for source repository: branch=[$SOURCE_BRANCH], commit=$SOURCE_COMMIT"; fi

if [ "$1" != "--installer" ]; then echo "Creating data package in $RELEASEDIR"; else echo "Creating data directory $RELEASEDIR"; fi

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
git archive --remote ${REPODIR} HEAD:docs | tar x

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
if [ "$1" != "--installer" ]; then
    echo "creating data archive: $PACKAGE"
    [[ -f "${RELEASEDIR_ROOT}/$PACKAGE" ]] && rm "${RELEASEDIR_ROOT}/$PACKAGE"
    cd $RELEASEDIR
    if [ "$ARCHIVE_TYPE" = "7z" ] && [ "$(which 7za 2>/dev/null)" != "" ]; then
	cd ..
	7za a "$PACKAGE" "$RELEASEDIR" >/dev/null
    else
	tar -cf - * | xz > ../$PACKAGE
    fi
    cd $CURRENTDIR

    ls -la ${RELEASEDIR_ROOT}/$PACKAGE
fi
if [ "$1" = "-CI" ] && [ -d "$RELEASEDIR" ]; then rm -rf "$RELEASEDIR"; fi
