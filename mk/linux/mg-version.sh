#!/bin/sh
# Use this script to idenitfy previous and current Version for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

KERNEL="$(uname -s | tr '[A-Z]' '[a-z]')"
if [ "$KERNEL" = "darwin" ]; then
	CURRENTDIR="$(cd "$(dirname "$0")"; pwd)"
else
	CURRENTDIR="$(dirname "$(readlink -f "$0")")"
fi
VERSION_INFO_FILE="$CURRENTDIR/../../source/version.txt"
OLD_MG_VERSION="$(awk -F '=' '/^OldReleaseGameVersion =/ {print $2}' "$VERSION_INFO_FILE" | awk -F '"' '{print $2}')"
OLD_MG_VERSION_BINARY="$OLD_MG_VERSION"
MG_VERSION="$(awk -F '=' '/^CurrentGameVersion =/ {print $2}' "$VERSION_INFO_FILE" | awk -F '"' '{print $2}')"

if [ "$1" = "--oldversion" ]; then
  echo "$OLD_MG_VERSION"
elif [ "$1" = "--oldversion_binary" ]; then
  echo "$OLD_MG_VERSION_BINARY"
elif [ "$1" = "--version" ]; then
  echo "$MG_VERSION"
fi
