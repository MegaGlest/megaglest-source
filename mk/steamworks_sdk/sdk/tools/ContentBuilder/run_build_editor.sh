#!/usr/bin/env bash

STEAMROOT="$(cd "${0%/*}" && echo $PWD)"
STEAMCMD=`basename "$0" .sh`

cp -p content/linux_x64/megaglest_editor content/editor_linux_x64/
cp -p content/linux_x64/glest.ini content/editor_linux_x64/

./builder_linux/steamcmd.sh +login megaglest_team $1 +run_app_build_http ../scripts/editor_app_linux_611990.vdf +quit
