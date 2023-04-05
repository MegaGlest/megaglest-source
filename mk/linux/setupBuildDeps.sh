#!/bin/bash
#
# Use this script to install build dependencies on a number of Linux platforms
# ----------------------------------------------------------------------------
# Originally written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Rewritten by Tom Reynolds <tomreyn@megaglest.org>
# Copyright (c) 2012-2017 Mark Vejvoda, Tom Reynolds under GNU GPL v3.0
LANG=C

SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

# If you wanna only see the BuildDeps then launch script like that: './setupBuildDeps.sh --manually'.
# If you wanna see the BuildDeps for other linux distribution (mentioned in this script) then e.g.
# './setupBuildDeps.sh --manually "Debian"' is for you.
# If you wanna see the BuildDeps for other linux distribution (mentioned in this script) and for
# specific release (mentioned in this script) then e.g.
# './setupBuildDeps.sh --manually "Debian" "stable"' is for you.

# Load shared functions
. $SCRIPTDIR/mg_shared.sh

# Got root?
if [ `id -u`'x' != '0x' ] && [ "$1" != "--manually" ]; then
	echo 'This script should be run as root (UID 0).' >&2
	exit 1
fi

if [ "$(which git 2>/dev/null)" != "" ]; then
	gitcommit="$(git log -1 --pretty=tformat:"%H" $SCRIPTDIR/../..)"
else
	gitcommit="- 'git' tool not available -"
fi

# Included from shared functions
detect_system

# Allow for quiet/silent installs or manual installation
if [ "$1" = "-q" ] || [ "$1" = "--quiet" ] || [ "$1" = "--silent" ]; then
	quiet=1; manual_install=0
elif [ "$1" = "--manually" ]; then
	quiet=0; manual_install=1
	if [ "$2" != "" ]; then
		distribution="$2"; codename="-"; lsb="-"
		if [ "$3" != "" ]; then release="$3"; else release="-"; fi
	fi
else
	quiet=0; manual_install=0
fi

if [ "$manual_install" = "0" ]; then
	echo 'We have detected the following system:'
	echo ' [ '"$distribution"' ] [ '"$release"' ] [ '"$codename"' ] [ '"$architecture"' ]'
	echo; echo 'On supported systems, we will now install build dependencies.'; echo
fi

common_info() {
	echo
	if [ "$1" != "no_install" ]; then
		echo 'Please report a bug at http://bugs.megaglest.org providing the following information:'
	fi
	echo '--- snip ---'
	echo 'Git revision: '"$gitcommit"
	echo 'LSB support:  '"$lsb"
	echo 'Distribution: '"$distribution"
	echo 'Release:      '"$release"
	echo 'Codename:     '"$codename"
	echo 'Architecture: '"$architecture"
	echo '--- snip ---'
	echo
	if [ "$1" = "+wiki" ]; then
		echo 'For now, you may want to take a look at the build hints on the MegaGlest wiki at:'
		echo '  https://docs.megaglest.org/MG/Linux_Compiling'
		echo 'If you can come up with something which works for you, please report back to us, too. Thanks!'
		exit 1
	fi
}
unsupported_distribution () {
	echo 'Unsupported Linux distribution.' >&2
	common_info +wiki
}
unsupported_release () {
	echo 'Unsupported '"$distribution"' release.' >&2
	common_info
	if [ "$installcommand" != '' ]; then
		echo 'For now, please try this (which works with other '"$distribution"' releases) and report back how it works for you:'; echo
		echo "$installcommand"; echo
		echo 'Thanks!'
	fi
	exit 1
}
install_command () {
	if [ "$installcommand" = '' ]; then echo 'Error detected: Empty install command!' >&2; exit 1; fi
	if [ "$manual_install" = "1" ]; then
		echo 'Proposed command to use:'; echo
		echo "$installcommand"
		common_info no_install
	else
		$installcommand
		if [ "$?" -ne "0" ]; then
			echo 'An error occurred while installing build dependencies.' >&2
			common_info +wiki
		fi
	fi
}

if [ "$quiet" -eq "1" ]; then
	APT_OPTIONS="$APT_OPTIONS -y -q"
	URPMI_OPTIONS="$URPMI_OPTIONS -q --auto"
	PACMAN_OPTIONS="$PACMAN_OPTIONS -q --noconfirm"
	DNF_OPTIONS="$DNF_OPTIONS -y -q"
	ZYPPER_OPTIONS="$ZYPPER_OPTIONS -y"
fi

packages_for_next_debian_ubuntu_mint="build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-0-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"

case $distribution in
	Debian|Raspbian)
		case $release in
			oldstable|8|8.*)
				#name > jessie, EoL May 2020
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.2-dev libjpeg-dev libpng12-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev"
				;;
			stable|9|9.*)
				#name > stretch, EoL ? May 2022
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-0-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"
				;;
			testing|unstable|10|10.0|11|11.0)
				#name > buster / sid
				#numbers for testing ...and for "next testing" too
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				;;
			*)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_currently_this_OS="release"
				;;
		esac
		;;

	Ubuntu)
		case $release in
			14.04.2|14.04.3|14.04.4)
				# "not so LTS" are those LTS v xD
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_currently_this_OS="release"
				;;
			14.04*)
				#LTS, name > trusty, EoL April 2019
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libsdl2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxgtk3.0-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libvlccore-dev libcppunit-dev"
				;;
			16.04*)
				#LTS, name > xenial, EoL April 2021
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng12-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"
				;;
			17.04)
				#name > zesty, EoL January 2018
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"
				;;
			17.10)
				#name > zesty, EoL January 2018
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev libidn2-dev libpsl-dev"
				;;
			"18.04"|"20.04")
				#name > Bionic, Focal Fossa
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh-dev libbrotli-dev"
				;;
            22.04)
				#name > Jammy Jellyfish
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh-dev libbrotli-dev libzstd-dev"
				;;
			*)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_currently_this_OS="release"
				;;
		esac
		;;
    Pop)
		case $release in
			20.04)
				#name > Focal Fossa
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh-dev libbrotli-dev"
				;;
			*)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_currently_this_OS="release"
				;;
		esac
		;;

	LinuxMint|Linuxmint)
		case $release in
			2)
				#LMDE 2, related with Debian ~ 8/jessie
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.2-dev libjpeg-dev libpng12-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev"
				;;
			17|17.*)
				#LTS, based on Ubuntu 14.04, EoL April 2019
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libsdl2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxgtk3.0-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev libftgl-dev libfribidi-dev libvlc-dev libvlccore-dev libcppunit-dev"
				;;
			18|18.*)
				#LTS, based on Ubuntu 16.04, EoL April 2021
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng12-dev libfreetype6-dev libwxgtk3.0-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn11-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev"
				;;
			21|21.*)
				#LTS, based on Ubuntu 22.04, EoL April 2027
				installcommand="apt-get install $APT_OPTIONS build-essential cmake libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libvlc-dev libvlccore-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-0-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh2-1-dev libzstd-dev libssh-dev"
				;;
			*)
				installcommand="apt-get install $APT_OPTIONS $packages_for_next_debian_ubuntu_mint"
				unsupported_currently_this_OS="release"
				;;
		esac
		;;

	SuSE|SUSE?LINUX|Opensuse*|openSUSE*)
		case $release in
			*)
				installcommand="zypper install $ZYPPER_OPTIONS gcc gcc-c++ cmake libSDL2-devel Mesa-libGL-devel freeglut-devel libvorbis-devel wxWidgets-devel lua-devel libjpeg8-devel libpng16-devel libcurl-devel openal-soft-devel libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel libminiupnpc-devel vlc-devel"
				unsupported_currently_this_OS="release"
				;;
		esac
		;;

	Fedora)
		case $release in
			*)
				installcommand="dnf $DNF_OPTIONS install gcc gcc-c++ redhat-rpm-config cmake SDL2-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel libX11-devel libxml2-devel libircclient-devel glew-devel ftgl-devel fribidi-devel cppunit-devel miniupnpc-devel"
				unsupported_currently_this_OS="release"
				;;
		esac
		;;

	Mageia)
		if [ "$architecture" = "x86_64" ]; then lib="lib64"; else lib="lib"; fi
		static="-static" #static=""
		case $release in
			*)
				installcommand="urpmi $URPMI_OPTIONS gcc gcc-c++ cmake make ${lib}curl-devel ${lib}sdl2.0-devel ${lib}openal-devel ${lib}lua${static}-devel ${lib}jpeg${static}-devel ${lib}png-devel ${lib}freetype6${static}-devel ${lib}wxgtku3.0-devel ${lib}cppunit-devel ${lib}fribidi${static}-devel ${lib}ftgl-devel ${lib}glew-devel ${lib}ogg${static}-devel ${lib}vorbis-devel ${lib}miniupnpc-devel ${lib}ircclient-static-devel ${lib}vlc-devel ${lib}xml2-devel ${lib}x11-devel ${lib}mesagl1-devel ${lib}mesaglu1-devel ${lib}openssl${static}-devel"
				unsupported_currently_this_OS="release"
				;;
		esac
		;;

	ManjaroLinux*|Manjarolinux*)
		if [ "$architecture" = "x86_64" ] || [ "$architecture" = "aarch64" ]; then lib=""; else lib="lib32-"; fi
		case $release in
			*)
				installcommand="pacman $PACMAN_OPTIONS -S --needed gcc-multilib cmake ${lib}libcurl-gnutls ${lib}sdl2 ${lib}openal lua ${lib}libjpeg-turbo ${lib}libpng ${lib}freetype2 wxwidgets-gtk3 cppunit fribidi ftgl ${lib}glew ${lib}libogg ${lib}libvorbis miniupnpc libircclient vlc ${lib}libxml2 ${lib}libx11 ${lib}mesa ${lib}glu"
				# unsupported_currently_this_OS="release"
		esac
		;;

	*)
		unsupported_currently_this_OS="distribution"
		;;
esac
if [ "$unsupported_currently_this_OS" = "release" ]; then unsupported_release
	elif [ "$unsupported_currently_this_OS" = "distribution" ]; then unsupported_distribution
	else install_command; fi
if [ "$manual_install" = "0" ]; then
	echo; echo 'Installation of build dependencies complete.'
fi
