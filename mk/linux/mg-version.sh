#!/bin/bash

OLD_MG_VERSION=3.5.2
MG_VERSION=3.5.3

if [ "$1" = "--oldversion" ]; then
  echo "$OLD_MG_VERSION"
elif [ "$1" = "--version" ]; then
  echo "$MG_VERSION"
fi
