#!/bin/sh
# Use this script to idenitfy previous and current Version for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

VERSION_INFO_FILE="$(dirname "$(readlink -f "$0")")/../../source/version.txt"
OLD_MG_VERSION="$(awk -F '=' '/^OldReleaseGameDataVersion =/ {print $2}' "$VERSION_INFO_FILE" | awk -F '"' '{print $2}')"
OLD_MG_VERSION_BINARY="$(awk -F '=' '/^OldReleaseGameBinaryVersion =/ {print $2}' "$VERSION_INFO_FILE" | awk -F '"' '{print $2}')"
MG_VERSION="$(awk -F '=' '/^CurrentGameVersion =/ {print $2}' "$VERSION_INFO_FILE" | awk -F '"' '{print $2}')"

if [ "$1" = "--oldversion" ]; then
  echo "$OLD_MG_VERSION"
elif [ "$1" = "--oldversion_binary" ]; then
  echo "$OLD_MG_VERSION_BINARY"
elif [ "$1" = "--version" ]; then
  echo "$MG_VERSION"
fi
