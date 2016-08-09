#!/bin/bash
#
# Use this script to install build dependencies on a number of Linux platforms
# ----------------------------------------------------------------------------
# Originally written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Rewritten by Tom Reynolds <tomreyn@megaglest.org>
# Copyright (c) 2012-2016 Mark Vejvoda, Tom Reynolds under GNU GPL v3.0
LANG=C

SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

# Load shared functions
. $SCRIPTDIR/mg_shared.sh

# Got root?
if [ `id -u`'x' != '0x' ]
then
	echo 'This script must be run as root (UID 0).' >&2
	exit 1
fi

if [ "$(which git 2>/dev/null)" != "" ]; then
	gitcommit="$(git log -1 --pretty=tformat:"%H" $SCRIPTDIR/../..)"
else
	gitcommit="- 'git' tool not available -"
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

common_info() {
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
	if [ "$1" = "+wiki" ]; then
		echo 'For now, you may want to take a look at the build hints on the MegaGlest wiki at:'
		echo '  https://docs.megaglest.org/MG/Linux_Compiling'
		echo 'If you can come up with something which works for you, please report back to us, too. Thanks!'
	fi
}
unsupported_distribution () {
	echo 'Unsupported Linux distribution.' >&2
	common_info +wiki
}
unsupported_release () {
	echo 'Unsupported '"$distribution"' release.' >&2
	common_info
	if [ "$installcommand" != '' ]
	then
		echo 'For now, please try this (which works with other '"$distribution"' releases) and report back how it works for you:'
		echo $installcommand
		echo 'Thanks!'
	fi
}
error_during_installation () {
	echo 'An error occurred while installing build dependencies.' >&2
	common_info +wiki
}

if [ "$quiet" -eq "1" ]; then
	APT_OPTIONS="$APT_OPTIONS -y -q"
	URPMI_OPTIONS="$URPMI_OPTIONS -q --auto"
	PACMAN_OPTIONS="$PACMAN_OPTIONS -q --noconfirm"
	DNF_OPTIONS="$DNF_OPTIONS -y -q"
fi

packages_for_next_debian_ubuntu_mint="build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"

case $distribution in
	Debian)
		case $release in
			oldstable|7|7.*)
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libsdl2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libminiupnpc-dev librtmp-dev libgtk2.0-dev libcppunit-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			stable|8|8.*)
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.2-dev libjpeg-dev libpng12-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			testing|unstable|9|9.0)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			*)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_release; exit 1
				;;
		esac
		;;

	Ubuntu)
		case $release in
			12.04.2|12.04.3|12.04.4)
				# "not so LTS" are those LTS v xD
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_release
				exit 1
				;;
			12.04*)
				#LTS, name > precise
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libcppunit-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			14.04*)
				#LTS, name > trusty
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libsdl2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libvlccore-dev libcppunit-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			16.04*)
				#LTS, name > xenial
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng12-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			*)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_release
				exit 1
				;;
		esac
		;;

	LinuxMint)
		case $release in
			13|13.*)
				#LTS, based on Ubuntu 12.04
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libsdl2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libcppunit-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			17|17.*)
				#LTS, based on Ubuntu 14.04
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libsdl2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libvlccore-dev libcppunit-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			18|18.*)
				#LTS, based on Ubuntu 16.04
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng12-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			*)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_release
				exit 1
				;;
		esac
		;;

	SuSE|SUSE?LINUX|Opensuse*|openSUSE*)
		case $release in
			13.1)
				#LTS
				installcommand="zypper install gcc gcc-c++ cmake libSDL2-devel Mesa-libGL-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel xorg-x11-libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			42.1)
				installcommand="zypper install gcc gcc-c++ cmake libSDL2-devel Mesa-libGL-devel freeglut-devel libvorbis-devel wxWidgets-devel lua-devel libjpeg8-devel libpng16-devel libcurl-devel openal-soft-devel libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel libminiupnpc-devel vlc-devel"
				$installcommand
				if [ "$?" -ne "0" ]; then error_during_installation; exit 1; fi
				;;
			*)
				installcommand="zypper install gcc gcc-c++ cmake libSDL2-devel Mesa-libGL-devel freeglut-devel libvorbis-devel wxWidgets-devel lua-devel libjpeg8-devel libpng16-devel libcurl-devel openal-soft-devel libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel libminiupnpc-devel vlc-devel"
				unsupported_release
				exit 1
				;;
		esac
		;;

	Fedora)
		case $release in
			*)
				installcommand="dnf $DNF_OPTIONS install gcc gcc-c++ redhat-rpm-config cmake SDL2-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel miniupnpc-devel"
				unsupported_release
				exit 1
				;;
		esac
		;;

	Mageia)
		if [ "$architecture" = "x86_64" ]; then lib="lib64"; else lib="lib"; fi
		static="-static" #static=""
		case $release in
			*)
				installcommand="urpmi $URPMI_OPTIONS gcc gcc-c++ cmake make ${lib}curl-devel ${lib}sdl2.0-devel ${lib}openal-devel ${lib}lua${static}-devel ${lib}jpeg${static}-devel ${lib}png-devel ${lib}freetype6${static}-devel ${lib}wxgtku3.0-devel ${lib}cppunit-devel ${lib}fribidi${static}-devel ${lib}ftgl-devel ${lib}glew-devel ${lib}ogg${static}-devel ${lib}vorbis-devel ${lib}miniupnpc-devel ${lib}ircclient-static-devel ${lib}vlc-devel ${lib}xml2-devel ${lib}x11-devel ${lib}mesagl1-devel ${lib}mesaglu1-devel ${lib}openssl${static}-devel"
				unsupported_release
				exit 1
				;;
		esac
		;;

	ManjaroLinux)
		if [ "$architecture" = "x86_64" ]; then lib=""; else lib="lib32-"; fi
		case $release in
			*)
				installcommand="pacman $PACMAN_OPTIONS -S --needed gcc-multilib cmake ${lib}libcurl-gnutls ${lib}sdl2 ${lib}openal lua ${lib}libjpeg-turbo ${lib}libpng ${lib}freetype2 ${lib}wxgtk cppunit fribidi ftgl ${lib}glew ${lib}libogg ${lib}libvorbis miniupnpc libircclient vlc ${lib}libxml2 ${lib}libx11 ${lib}mesa ${lib}glu"
				unsupported_release
				exit 1
		esac
		;;

	*)
		unsupported_distribution
		exit 1
		;;
esac

echo ''
echo 'Installation of build dependencies complete.'
