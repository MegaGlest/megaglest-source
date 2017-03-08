#!/bin/sh
# Use this script to synchronize other scripts and installers with the version 
# in mg-version.sh for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011-2015 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"
CURRENT_VERSION=`./mg-version.sh --version`
OLD_VERSION=`./mg-version.sh --oldversion_binary`
VERSION_INFO_FILE="$(dirname "$(readlink -f "$0")")/../../source/version.txt"
LastCompatibleSaveGameVersion="$(awk -F '=' '/^LastCompatibleSaveGameVersion =/ {print $2}' "$VERSION_INFO_FILE" | awk -F '"' '{print $2}')"
CurrYear="$(date +%y)"
MapeditorVersion="$CURRENT_VERSION"
G3dviewerVersion="$CURRENT_VERSION"
modifymore="yes"
echo
echo '===== Updating Game ======'
# const string glestVersionString 	= "v3.12-dev";
echo 'Game version # before:'
grep -E '^const string glestVersionString 	= "[^"]*";$' ../../source/glest_game/facilities/game_util.cpp
sed -i 's/^const string glestVersionString 	= "[^"]*";$/const string glestVersionString 	= "v'$CURRENT_VERSION'";/' ../../source/glest_game/facilities/game_util.cpp
echo 'Game version # after:'
grep -E '^const string glestVersionString 	= "[^"]*";$' ../../source/glest_game/facilities/game_util.cpp
echo
echo 'Game Copyright date # before:'
grep -E 'Copyright 2010-20[0-9][0-9] The MegaGlest Team' ../../source/glest_game/facilities/game_util.cpp
sed -i 's/Copyright 2010-20[0-9][0-9] The MegaGlest Team/Copyright 2010-20'$CurrYear' The MegaGlest Team/' ../../source/glest_game/facilities/game_util.cpp
grep -E '© 2001-20[0-9][0-9] The MegaGlest Team' ../../mk/macos/bundle_resources/Info.plist.in
sed -i 's/© 2001-20[0-9][0-9] The MegaGlest Team/© 2001-20'$CurrYear' The MegaGlest Team/' ../../mk/macos/bundle_resources/Info.plist.in
echo 'Game Copyright date # after:'
grep -E 'Copyright 2010-20[0-9][0-9] The MegaGlest Team' ../../source/glest_game/facilities/game_util.cpp
grep -E '© 2001-20[0-9][0-9] The MegaGlest Team' ../../mk/macos/bundle_resources/Info.plist.in
echo
if [ "$modifymore" = "yes" ]; then
	# const string lastCompatibleSaveGameVersionString 	= "v3.9.0";
	echo 'Compatible Save Game version # before:'
	grep -E '^const string lastCompatibleSaveGameVersionString 	= "[^"]*";$' ../../source/glest_game/facilities/game_util.cpp
	sed -i 's/^const string lastCompatibleSaveGameVersionString 	= "[^"]*";$/const string lastCompatibleSaveGameVersionString 	= "v'$LastCompatibleSaveGameVersion'";/' ../../source/glest_game/facilities/game_util.cpp
	echo 'Compatible Save Game version # after:'
	grep -E '^const string lastCompatibleSaveGameVersionString 	= "[^"]*";$' ../../source/glest_game/facilities/game_util.cpp
	echo
fi
if [ "$modifymore" = "yes" ] && [ "$(git status >/dev/null 2>&1; echo "$?")" -eq "0" ]; then
	# const string GIT_RawRev		= "$4446.1a8673f$";
	GitCommitForRelease="`git rev-list HEAD --count`.`git log -1 --format=%h --abbrev=7`";
	echo 'GitCommitForRelease # before:'
	grep -E '^GitCommitForRelease = "[^"]*";$' "$VERSION_INFO_FILE"
	grep -E '^	const string GIT_RawRev		= "\$[^"$]*\$";$' ../../source/glest_game/facilities/game_util.cpp
	sed -i 's/^GitCommitForRelease = "[^"]*";$/GitCommitForRelease = "'$GitCommitForRelease'";/' "$VERSION_INFO_FILE"
	sed -i 's/^	const string GIT_RawRev		= "$[^"]*";$/	const string GIT_RawRev		= "$'$GitCommitForRelease'$";/' ../../source/glest_game/facilities/game_util.cpp
	echo 'GitCommitForRelease # after:'
	grep -E '^GitCommitForRelease = "[^"]*";$' "$VERSION_INFO_FILE"
	grep -E '^	const string GIT_RawRev		= "\$[^"$]*\$";$' ../../source/glest_game/facilities/game_util.cpp
fi
echo
if [ "$modifymore" = "yes" ]; then
	echo '===== Updating Mapeditor ======'
	# const string mapeditorVersionString = "v1.6.1";
	echo 'Mapeditor version # before:'
	grep -E '^const string mapeditorVersionString = "[^"]*";$' ../../source/glest_map_editor/main.cpp
	sed -i 's/^const string mapeditorVersionString = "[^"]*";$/const string mapeditorVersionString = "v'$MapeditorVersion'";/' ../../source/glest_map_editor/main.cpp
	echo 'Mapeditor version # after:'
	grep -E '^const string mapeditorVersionString = "[^"]*";$' ../../source/glest_map_editor/main.cpp
	echo
	echo 'Mapeditor Copyright date # before:'
	grep -E 'Copyright 2010-20[0-9][0-9] The MegaGlest Team' ../../source/glest_map_editor/main.cpp
	sed -i 's/Copyright 2010-20[0-9][0-9] The MegaGlest Team/Copyright 2010-20'$CurrYear' The MegaGlest Team/' ../../source/glest_map_editor/main.cpp
	echo 'Mapeditor Copyright date # after:'
	grep -E 'Copyright 2010-20[0-9][0-9] The MegaGlest Team' ../../source/glest_map_editor/main.cpp
	echo
	echo '===== Updating G3dviewer ======'
	# const string g3dviewerVersionString= "v1.3.6";
	echo 'G3dviewer version # before:'
	grep -E '^const string g3dviewerVersionString= "[^"]*";$' ../../source/g3d_viewer/main.cpp
	sed -i 's/^const string g3dviewerVersionString= "[^"]*";$/const string g3dviewerVersionString= "v'$G3dviewerVersion'";/' ../../source/g3d_viewer/main.cpp
	echo 'G3dviewer version # after:'
	grep -E '^const string g3dviewerVersionString= "[^"]*";$' ../../source/g3d_viewer/main.cpp
fi
echo
echo '===== Updating Linux Installer ======'
# local GAME_VERSION = "x.x.x";
echo 'Linux Installer version # before:'
grep -E '^local GAME_VERSION = "[^"]*";$' tools-for-standalone-client/installer/scripts/config.lua
sed -i 's/^local GAME_VERSION = "[^"]*";$/local GAME_VERSION = "'$CURRENT_VERSION'";/' tools-for-standalone-client/installer/scripts/config.lua
echo 'Linux Installer version # after:'
grep -E '^local GAME_VERSION = "[^"]*";$' tools-for-standalone-client/installer/scripts/config.lua
echo
echo '===== Updating Windows Installer ======'
# !define APVER 3.6.0
echo 'Windows Installer version # before:'
grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi
sed -i 's/^\!define APVER [^"]*$/\!define APVER '$CURRENT_VERSION'/' ../windoze/Installer/MegaGlestInstaller.nsi
echo 'Windows Installer version # after:'
grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi
echo
if [ "$modifymore" = "yes" ]; then
	echo 'Windows Installer version # before:'
	grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi
	sed -i 's/^\!define APVER_OLD [^"]*$/\!define APVER_OLD '$OLD_VERSION'/' ../windoze/Installer/MegaGlestInstaller.nsi
	echo 'Windows Installer version # after:'
	grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi
	echo
	echo '===== Updating Windows Updater ======'
	echo 'Windows Updater version # before:'
	grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi
	sed -i 's/^\!define APVER [^"]*$/\!define APVER '$CURRENT_VERSION'/' ../windoze/Installer/MegaGlestUpdater.nsi
	echo 'Windows Updater version # after:'
	grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi
	echo
	echo 'Windows Updater version # before:'
	grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi
	sed -i 's/^\!define APVER_OLD [^"]*$/\!define APVER_OLD '$OLD_VERSION'/' ../windoze/Installer/MegaGlestUpdater.nsi
	echo 'Windows Updater version # after:'
	grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi
	echo
	echo 'Windows Updater version # before:'
	grep -E '^\!define APVER_UPDATE [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi
	sed -i 's/^\!define APVER_UPDATE [^"]*$/\!define APVER_UPDATE '$CURRENT_VERSION'/' ../windoze/Installer/MegaGlestUpdater.nsi
	echo 'Windows Updater version # after:'
	grep -E '^\!define APVER_UPDATE [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi
	echo
fi
