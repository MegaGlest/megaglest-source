#!/bin/bash
#
# Use this script to install build dependencies on a number of Linux platforms
# ----------------------------------------------------------------------------
# Originally written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Rewritten by Tom Reynolds <tomreyn@megaglest.org>
# Copyright (c) 2012-2016 Mark Vejvoda, Tom Reynolds under GNU GPL v3.0

SCRIPTDIR="$(dirname $(readlink -f $0))"

# Load shared functions
. $SCRIPTDIR/../../../mk/linux/mg_shared.sh

echo 'Downloading Javascript libraries...'
wget http://code.jquery.com/jquery-1.12.0.js -O media/jquery-1.12.0.min.js
wget http://www.datatables.net/download/build/jquery.dataTables.min.js -O media/jquery.dataTables.min.js

echo 'Detecting system and installing dependencies...'
# Included from shared functions
detect_system

echo 'We have detected the following system:'
echo ' [ '"$distribution"' ] [ '"$release"' ] [ '"$codename"' ] [ '"$architecture"' ]'

case $distribution in
	SuSE|SUSE?LINUX|Opensuse)
		case $release in
			*)
				echo '(Open)SuSE...'
				sudo zypper install graphviz-perl perl-GraphViz perl-Config-IniFiles perl-PerlMagick
				;;
		esac
		;;

	Debian|Ubuntu|LinuxMint)
		case $release in
			*)
				echo 'Debian / Ubuntu...'
				sudo apt-get install perl graphviz libgraphviz-perl libconfig-inifiles-perl perlmagick
				#sudo apt-get install libimage-size-perl
				;;
		esac
		;;
	*)
		echo 'ERROR: Unknown distribution. Stopping here.' >&2
		exit 1
esac

echo ''
echo 'To create techtree HTML pages copy / edit megapack.ini and run the script:'
echo './convert_faction_xml2html.pl megapack.ini'
