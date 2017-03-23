#!/usr/bin/env bash

STEAMROOT="$(cd "${0%/*}" && echo $PWD)"
STEAMCMD=`basename "$0" .sh`

./builder_linux/steamcmd.sh +login megaglest_team $1 +run_app_build_http ../scripts/editor_app_win_611990.vdf +quit
