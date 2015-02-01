#/bin/bash
# This is a help script to build windows installers on a linux machine
#
# for this script nsis is needed:
# windows binaries must be in place ( for example from snapshots.megaglest.org )

CURRENTDIR="$(dirname $(readlink -f $0))"
cd "$CURRENTDIR"

# We need the AccessControl plugin for nsis and place this in plugins directory
# original from http://nsis.sourceforge.net/AccessControl_plug-in
wget http://downloads.megaglest.org/windowsInstallerHelp/AccessControl.dll -P plugins
wget http://downloads.megaglest.org/windowsInstallerHelp/AccessControl.txt -P plugins

makensis MegaGlestInstaller.nsi

