#!/bin/bash

OLD_MG_VERSION=3.6.0.1
MG_VERSION=3.6.1-dev

if [ "$1" = "--oldversion" ]; then
  echo "$OLD_MG_VERSION"
elif [ "$1" = "--version" ]; then
  echo "$MG_VERSION"
fi
