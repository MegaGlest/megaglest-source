#!/bin/bash

OLD_MG_VERSION=3.3.7.2
MG_VERSION=3.4.0

if [ "$1" = "--oldversion" ]; then
  echo "$OLD_MG_VERSION"
elif [ "$1" = "--version" ]; then
  echo "$MG_VERSION"
fi
