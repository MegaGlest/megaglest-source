#!/bin/bash
# Use this script to synchronize other scripts and installers with the version 
# in mg-version.sh for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

CURRENT_VERSION=`./mg-version.sh --version`
OLD_VERSION=`./mg-version.sh --oldversion_binary`

echo '===== Updating Linux Installer ======'
# local GAME_VERSION = "x.x.x";
echo 'Linux Installer version # before:'
grep -E '^local GAME_VERSION = "[^"]*";$' mojosetup/megaglest-installer/scripts/config.lua;sed -i 's/^local GAME_VERSION = "[^"]*";$/local GAME_VERSION = "'$CURRENT_VERSION'";/' mojosetup/megaglest-installer/scripts/config.lua
echo 'Linux Installer version # after:'
grep -E '^local GAME_VERSION = "[^"]*";$' mojosetup/megaglest-installer/scripts/config.lua

echo '===== Updating Windows Installer ======'
# !define APVER 3.6.0
echo 'Windows Installer version # before:'
grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi;sed -i 's/^\!define APVER [^"]*$/\!define APVER '$CURRENT_VERSION'/' ../windoze/Installer/MegaGlestInstaller.nsi
echo 'Windows Installer version # after:'
grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi

echo 'Windows Installer version # before:'
grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi;sed -i 's/^\!define APVER_OLD [^"]*$/\!define APVER_OLD '$OLD_VERSION'/' ../windoze/Installer/MegaGlestInstaller.nsi
echo 'Windows Installer version # after:'
grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestInstaller.nsi

echo '===== Updating Windows Updater ======'
echo 'Windows Updater version # before:'
grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi;sed -i 's/^\!define APVER [^"]*$/\!define APVER '$CURRENT_VERSION'/' ../windoze/Installer/MegaGlestUpdater.nsi
echo 'Windows Updater version # after:'
grep -E '^\!define APVER [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi

echo 'Windows Updater version # before:'
grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi;sed -i 's/^\!define APVER_OLD [^"]*$/\!define APVER_OLD '$OLD_VERSION'/' ../windoze/Installer/MegaGlestUpdater.nsi
echo 'Windows Updater version # after:'
grep -E '^\!define APVER_OLD [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi

grep -E '^\!define APVER_UPDATE [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi;sed -i 's/^\!define APVER_UPDATE [^"]*$/\!define APVER_UPDATE '$CURRENT_VERSION'/' ../windoze/Installer/MegaGlestUpdater.nsi
echo 'Windows Updater version # after:'
grep -E '^\!define APVER_UPDATE [^"]*$' ../windoze/Installer/MegaGlestUpdater.nsi


