#!/bin/sh
# Use this script in bundle to run game
# ----------------------------------------------------------------------------
# Copyright (c) 2015 under GNU GPL v3.0+

export LANG=C
SCRIPTDIR="$(cd "$(dirname "$0")"; pwd)"
export DYLD_LIBRARY_PATH="$SCRIPTDIR/../FRAMEWORKS"
export PATH="$SCRIPTDIR/../Resources/megaglest-game:$PATH"

exec "$SCRIPTDIR/../Resources/megaglest-game/megaglest"
exit "$?"
