#!/bin/bash
#
# Use this script to install build dependencies on a number of Linux platforms
# ----------------------------------------------------------------------------
# Originally written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Rewritten by Tom Reynolds <tomreyn@megaglest.org>
# Copyright (c) 2012 Mark Vejvoda, Tom Reynolds under GNU GPL v3.0


LANG=C

SCRIPTDIR="$(dirname $(readlink -f $0))"
gitcommit=`git log -1 --pretty=tformat:"%H" $SCRIPTDIR/../..`

# Load shared functions
. $SCRIPTDIR/mg_shared.sh

# Got root?
if [ `id -u`'x' != '0x' ]
then 
	echo 'This script must be run as root (UID 0).' >&2
	exit 1
fi

# Do you have the 'git' command?
if [ `which git`'x' = 'x' ] 
then
	echo 'Could not find "git", please make sure it is installed.' >&2
	exit 1
fi

# Allow for quiet/silent installs
if [ $1'x' = '-qx' -o $1'x' = '--quietx' -o $1'x' = '--silentx' ] 
then
	quiet=1
else
	quiet=0
fi


# Included from shared functions
detect_system

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
	echo 'Git revision: '"$gitcommit"
	echo 'LSB support:  '"$lsb"
	echo 'Distribution: '"$distribution"
	echo 'Release:      '"$release"
	echo 'Codename:     '"$codename"
	echo 'Architecture: '"$architecture"
	echo '--- snip ---'
	echo ''
	echo 'For now, you may want to take a look at the build hints on the MegaGlest wiki at http://wiki.megaglest.org/'
	echo 'If you can come up with something which works for you, please report back to us, too. Thanks!'
}

unsupported_release () {
	echo 'Unsupported '"$distribution"' release.' >&2
	echo ''
	echo 'Please report a bug at http://bugs.megaglest.org providing the following information:'
	echo '--- snip ---'
	echo 'Git revision: '"$gitcommit"
	echo 'LSB support:  '"$lsb"
	echo 'Distribution: '"$distribution"
	echo 'Release:      '"$release"
	echo 'Codename:     '"$codename"
	echo 'Architecture: '"$architecture"
	echo '--- snip ---'
	echo ''
	if [ "$installcommand" != '' ]
	then
		echo 'For now, please try this (which works with other '"$distribution"' releases) and report back how it works for you:'
		echo $installcommand
		echo 'Thanks!'
	fi
}

error_during_installation () {
	echo 'An error occurred while installing build dependencies.' >&2
	echo ''
	echo 'Please report a bugs at http://bugs.megaglest.org providing the following information:'
	echo '--- snip ---'
	echo 'Git revision: '"$gitcommit"
	echo 'LSB support:  '"$lsb"
	echo 'Distribution: '"$distribution"
	echo 'Release:      '"$release"
	echo 'Codename:     '"$codename"
	echo 'Architecture: '"$architecture"
	echo '--- snip ---'
	echo ''
	echo 'For now, you may want to take a look at the build hints on the MegaGlest wiki at http://wiki.megaglest.org/'
	echo 'If you can come up with something which works for you, please report back to us, too. Thanks!'
}



case $distribution in
	Debian)
		if [ $quiet = 1 ]; then
			APT_OPTIONS="$APT_OPTIONS -y -q"
		fi
		case $release in
			6.0*|unstable)
				# No libvlc-dev since version (1.1.3) in Debian 6.0/Squeeze is incompatible, no libluajit-5.1-dev because it is not available on Debian 6.0/Squeeze, cf. http://glest.org/glest_board/?topic=8460
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libminiupnpc-dev librtmp-dev libgtk2.0-dev libcppunit-dev'
				$installcommand
				if [ $? != 0 ]; then
					error_during_installation; 
					echo ''
					echo 'Be sure to have the squeeze-backports repository installed, it is required for libminiupnpc-dev.'
					exit 1; 
				fi
				;;
			*)
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libminiupnpc-dev librtmp-dev libgtk2.0-dev libcppunit-dev'
				unsupported_release
				exit 1
				;;
		esac
		;;

	Ubuntu) 
		if [ $quiet = 1 ]; then
			APT_OPTIONS="$APT_OPTIONS -y -q"
		fi
		case $release in
			8.04)
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libcppunit-dev'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			10.04)
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew1.5-dev libftgl-dev libfribidi-dev libcppunit-dev'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			11.10|12.04|12.10|13.04|13.10|14.04)
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libcppunit-dev'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			*)
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libcppunit-dev'
				unsupported_release
				exit 1
				;;
		esac
		;;

	LinuxMint) 
		if [ $quiet = 1 ]; then
			APT_OPTIONS="$APT_OPTIONS -y -q"
		fi
		case $release in

			13|14|15|16)
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libcppunit-dev'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			*)
				installcommand='apt-get install '"$APT_OPTIONS"' build-essential cmake libsdl1.2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libcppunit-dev'
				unsupported_release
				exit 1
				;;
		esac
		;;

	SuSE|SUSE?LINUX|Opensuse*|openSUSE*) 
		case $release in
			11.2|11.3|11.4|12.1)
				installcommand='zypper install gcc gcc-c++ cmake libSDL-devel MesaGLw-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng14-devel libcurl-devel openal-soft-devel xorg-x11-libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi 
				;;
			12.2)
				installcommand='zypper install gcc gcc-c++ cmake libSDL-devel Mesa-libGL-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng14-devel libcurl-devel openal-soft-devel xorg-x11-libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi
				;;
			13.1)
				installcommand='zypper install gcc gcc-c++ cmake libSDL-devel Mesa-libGL-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel xorg-x11-libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi
				;;
			*)
				installcommand='zypper install gcc gcc-c++ cmake libSDL-devel Mesa-libGL-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng14-devel libcurl-devel openal-soft-devel xorg-x11-libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel'
				unsupported_release
				exit 1
				;;
		esac
		;;

	Fedora) 
		case $release in
			13|14|15|16|17|18)
				installcommand='yum groupinstall development-tools'
				$installcommand
				if [ $? != 0 ]; then error_during_installation; exit 1; fi

				installcommand='yum install cmake SDL-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel'
				$installcommand
				;;
			*)
				installcommand='yum groupinstall "Development Tools"; yum install cmake SDL-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel'
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
