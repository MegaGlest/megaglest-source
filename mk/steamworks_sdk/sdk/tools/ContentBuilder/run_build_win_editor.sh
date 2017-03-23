#!/usr/bin/env bash

STEAMROOT="$(cd "${0%/*}" && echo $PWD)"
STEAMCMD=`basename "$0" .sh`

cp -p content/win_x86/megaglest_editor.exe content/editor_win_x86/
cp -p content/win_x86/glest.ini content/editor_win_x86/

cp -p content/win_x64/megaglest_editorx64.exe content/editor_win_x64/
cp -p content/win_x64/glest.ini content/editor_win_x64/

./builder_linux/steamcmd.sh +login megaglest_team $1 +run_app_build_http ../scripts/editor_app_win_611990.vdf +quit
