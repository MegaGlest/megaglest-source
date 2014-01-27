#!/bin/bash
# Use this script to idenitfy previous and current Version for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

OLD_MG_VERSION=3.9.1
OLD_MG_VERSION_BINARY=3.9.1
MG_VERSION=3.10.0-dev

if [ "$1" = "--oldversion" ]; then
  echo "$OLD_MG_VERSION"
elif [ "$1" = "--oldversion_binary" ]; then
  echo "$OLD_MG_VERSION_BINARY"
elif [ "$1" = "--version" ]; then
  echo "$MG_VERSION"
fi
