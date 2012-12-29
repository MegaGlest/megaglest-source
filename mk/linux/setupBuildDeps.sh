#!/bin/sh
#
# Use this script to install build dependencies on a number of Linux platforms
# ----------------------------------------------------------------------------
# Originally written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Rewritten by Tom Reynolds <tomreyn@megaglest.org>
# Copyright (c) 2012 Mark Vejvoda, Tom Reynolds under GNU GPL v3.0


LANG=C

# Got root?
if [ `id -u`'x' != '0x' ]
then 
	echo 'This script must be run as root (UID 0).' >&2
	exit 1
fi

# Do you have the 'svnversion' command?
if [ `which svnversion`'x' = 'x' ] 
then
	echo 'Could not find "svnversion", please make sure it is installed.' >&2
	exit 1
fi


svnversion=`readlink -f $0 | xargs dirname | xargs svnversion`
architecture=`uname -m`

# Is the lsb_release command supported?
if [ `which lsb_release`'x' = 'x' ]
then
	lsb=0
	if [ -e /etc/debian_version ];   then distribution='Debian';   release='unknown release version'; codename=`cat /etc/debian_version`;   fi
	if [ -e /etc/SuSE-release ];     then distribution='SuSE';     release='unknown release version'; codename=`cat /etc/SuSE-release`;     fi
	if [ -e /etc/fedora-release ];   then distribution='Fedora';   release='unknown release version'; codename=`cat /etc/fedora-release`;   fi
	if [ -e /etc/redhat-release ];   then distribution='Redhat';   release='unknown release version'; codename=`cat /etc/redhat-release`;   fi
	if [ -e /etc/mandrake-release ]; then distribution='Mandrake'; release='unknown release version'; codename=`cat /etc/mandrake-release`; fi
else
	lsb=1

	# lsb_release output by example:
        #
	# $ lsb_release -i
	# Distributor ID:       Ubuntu
	#
	# $ lsb_release -d
	# Description:  Ubuntu 12.04 LTS
	#
	# $ lsb_release -r
	# Release:      12.04
	#
	# $ lsb_release -c
	# Codename:     precise

	distribution=`lsb_release -i | awk -F':' '{ gsub(/^[ \t]*/,"",$2); print $2 }'`
	release=`lsb_release -r | awk -F':' '{ gsub(/^[  \t]*/,"",$2); print $2 }'`
	codename=`lsb_release -c | awk -F':' '{ gsub(/^[ \t]*/,"",$2); print $2 }'`

	# Some distribution examples:
	#
	# OpenSuSE 11.4
	#LSB Version:    n/a
	#Distributor ID: SUSE LINUX
	#Description:    openSUSE 11.4 (x86_64)
	#Release:        11.4
	#Codename:       Celadon
	#
	# OpenSuSE 12.1
	#LSB support:  1
	#Distribution: SUSE LINUX
	#Release:      12.1
	#Codename:     Asparagus
	#
	# Arch
	#LSB Version:    n/a
	#Distributor ID: archlinux
	#Description:    Arch Linux
	#Release:        rolling
	#Codename:       n/a
	#
	# Ubuntu 12.04
	#Distributor ID: Ubuntu
	#Description:	 Ubuntu 12.04 LTS
	#Release:	 12.04
	#Codename:	 precise
fi

echo 'We have detected the following system:'
echo ' [ '"$distribution"' ] [ '"$release"' ] [ '"$codename"' ] [ '"$architecture"' ]'
echo ''
echo 'On supported systems, we will now install build dependencies.'
echo ''

# Until this point you may cancel without any modifications applied 
#exit 0


unsupported_distribution () {
	echo 'Unsupported Linux distribution.' >&2
	echo ''
	echo 'Please report a bug at http://bugs.megaglest.org providing the following information:'
	echo '--- snip ---'
	echo 'SVN version:  '"$svnversion"
	echo 'LSB support:  '"$lsb"
	echo 'Distribution: '"$distribution"
	echo 'Release:      '"$release"
	echo 'Codename:     '"$codename"
	echo '--- snip ---'
	echo ''
	echo 'For now, you may want to take a look at the build hints on the MegaGlest wiki at http://wiki.megaglest.org/'
	echo 'If you can come up with something which works for you, please report back to us, too. Thanks!'
}

unsupported_release () {
	echo 'Unsupported '"$distribution"'release.' >&2
	echo ''
	echo 'Please report a bug at http://bugs.megaglest.org providing the following information:'
	echo '--- snip ---'
	echo 'SVN version:  '"$svnversion"
	echo 'LSB support:  '"$lsb"
	echo 'Distribution: '"$distribution"
	echo 'Release:      '"$release"
	echo 'Codename:     '"$codename"
	echo '--- snip ---'
	echo ''
	if [ "$alternative" != '' ]
	then
		echo 'For now, please try this (which works with other '"$distribution"' releases) and report back how it works for you:'
		echo $alternative
		echo 'Thanks!'
	fi
}

error_during_installation () {
	echo 'An error occurred while installing build dependencies.' >&2
	echo ''
	echo 'Please report a bugs at http://bugs.megaglest.org providing the following information:'
	echo '--- snip ---'
	echo 'SVN version:  '"$svnversion"
	echo 'LSB support:  '"$lsb"
	echo 'Distribution: '"$distribution"
	echo 'Release:      '"$release"
	echo 'Codename:     '"$codename"
	echo '--- snip ---'
	echo ''
	echo 'For now, you may want to take a look at the build hints on the MegaGlest wiki at http://wiki.megaglest.org/'
	echo 'If you can come up with something which works for you, please report back to us, too. Thanks!'
}



case $distribution in
	Debian) 
		case $release in
			6.0*)
				apt-get install build-essential subversion automake autoconf autogen cmake cmake-curses-gui
				if [ $? != 0 ]; then error_during_installation; exit 1; fi
				# No libvlc-dev since version (1.1.3) in Debian 6.0/Squeeze is incompatible, no libluajit-5.1-dev because it is not available on Debian 6.0/Squeeze, cf. http://glest.org/glest_board/?topic=8460
				apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libminiupnpc-dev librtmp-dev libgtk2.0-dev libfribidi-dev
				if [ $? != 0 ]; then
					error_during_installation; 
					echo ''
					echo 'Be sure to have the squeeze-backports repository installed, it is required for libminiupnpc-dev.'
					exit 1; 
				fi
				;;
			*)
				alternative='apt-get install build-essential subversion automake autoconf autogen cmake cmake-curses-gui; apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libminiupnpc-dev libvlc-dev librtmp-dev libgtk2.0-dev libfribidi-dev'
				unsupported_release
				exit 1
				;;
		esac
		;;

	Ubuntu) 
		case $release in
			8.04)
				apt-get install build-essential subversion automake autoconf autogen cmake
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			10.04)
				apt-get install build-essential subversion automake autoconf autogen cmake
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew1.5-dev libftgl-dev libfribidi-dev
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;

			11.10|12.04|12.10)
				apt-get install build-essential subversion automake autoconf autogen cmake
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libvlc-dev libfribidi-dev
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			*)
				alternative='apt-get install build-essential subversion automake autoconf autogen cmake; apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libvlc-dev libfribidi-dev'
				unsupported_release
				exit 1
				;;
		esac
		;;

	LinuxMint) 
		case $release in

			13)
				apt-get install build-essential subversion automake autoconf autogen cmake
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libvlc-dev libfribidi-dev
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			*)
				alternative='apt-get install build-essential subversion automake autoconf autogen cmake; apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libvlc-dev libfribidi-dev'
				unsupported_release
				exit 1
				;;
		esac
		;;

	SuSE|Opensuse) 
		case $release in
			11.2|11.3|11.4)
				zypper install subdistribution gcc gcc-c++ automake cmake
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				zypper install libSDL-devel libxerces-c-devel MesaGLw-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng14-devel libcurl-devel openal-soft-devel xorg-x11-libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			*)
				alternative='zypper install subdistribution gcc gcc-c++ automake cmake; zypper install libSDL-devel libxerces-c-devel MesaGLw-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng14-devel libcurl-devel openal-soft-devel xorg-x11-libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel'
				unsupported_release
				exit 1
				;;
		esac
		;;


	Fedora) 
		case $release in
			13|14)
				yum groupinstall "Development Tools"
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				yum install subdistribution automake autoconf autogen cmake
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				yum install SDL-devel xerces-c-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel libX11-devel libxml2-dev libircclient-dev glew-devel ftgl-devel fribidi-devel
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			*)
				alternative='yum groupinstall "Development Tools"; yum install subdistribution automake autoconf autogen cmake; yum install SDL-devel xerces-c-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel libX11-devel libxml2-dev libircclient-dev glew-devel ftgl-devel fribidi-devel'
				unsupported_release
				exit 1
				;;
		esac
		;;

	archlinux)
		case $release in
			rolling)
				unsupported_release
				exit 1
				;;
		esac
		;;

#	Redhat) 
#
#		;;
#
#	Mandrake|Mandriva) 
#
#		;;

	*) 
		unsupported_distribution
		exit 1
		;;
esac

echo ''
echo 'Installation of build dependencies complete.'
