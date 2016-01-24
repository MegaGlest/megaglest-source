#!/bin/sh
# Use this script in bundle to run game
# ----------------------------------------------------------------------------
# Copyright (c) 2015 under GNU GPL v3.0+

export LANG=C
SCRIPTDIR="$(cd "$(dirname "$0")"; pwd)"
if [ -d "$SCRIPTDIR/lib" ]; then
    export DYLD_LIBRARY_PATH="$SCRIPTDIR/lib"
    binary_dir_path="$SCRIPTDIR"
else
    export DYLD_LIBRARY_PATH="$SCRIPTDIR/../Frameworks"
    binary_dir_path="$SCRIPTDIR/../Resources/megaglest-game"
fi
export PATH="$binary_dir_path:$PATH"

exec "$binary_dir_path/megaglest" "$@"
exit "$?"
