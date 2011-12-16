#!/bin/bash
# Use this script to synchronize other scripts and installers with the version 
# in mg-version.sh for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

CURRENT_VERSION=`./mg-version.sh --version`

echo 'Linux Installer version # before:'
grep -E '^local GAME_VERSION = "[^"]*";$' mojosetup/megaglest-installer/scripts/config.lua;sed -i 's/^local GAME_VERSION = "[^"]*";$/local GAME_VERSION = "'$CURRENT_VERSION'";/' mojosetup/megaglest-installer/scripts/config.lua
echo 'Linux Installer version # after:'
grep -E '^local GAME_VERSION = "[^"]*";$' mojosetup/megaglest-installer/scripts/config.lua
