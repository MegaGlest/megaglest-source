#!/bin/bash
# script for use with travis and on linux only
#
# Copyright (c) 2015-2016 MegaGlest Team under GNU GPL v3.0+

export LANG=C
SCRIPTDIR="$(dirname "$(readlink -f "$0")")"
# ----------------------------------------------------------------------------
# Load shared functions
. $SCRIPTDIR/mk/linux/mg_shared.sh
detect_system
# ----------------------------------------------------------------------------
Compiler_name="$1"; Compiler_version="$2"
Compiler_version_grep="$(echo "$Compiler_version" | sed 's/\./\\./g')"

set -x
if [ "$Compiler_version" != "" ] && [ "$Compiler_version" != "default" ]; then
    # UPDATE REPOS
    sudo apt-get update -qq
    sudo apt-get install -y -qq

    set +x
    if [ "$Compiler_name" = "gcc" ]; then
	VersionAvByDefault="$(apt-cache search ^g[c+][c+]-[0-9] | grep -v '[0-9]-[a-zA-Z]' | grep "^gcc-$Compiler_version_grep")"
    elif [ "$Compiler_name" = "clang" ]; then
	VersionAvByDefault="$(apt-cache search ^clang-[0-9] | grep -v '[0-9]-[a-zA-Z]' | grep "^clang-$Compiler_version_grep")"
    fi
    set -x
    if [ "$VersionAvByDefault" = "" ]; then
	if [ "$distribution" = "Ubuntu" ]; then
	    if [ "$Compiler_name" = "gcc" ] || ( [ "$Compiler_name" = "clang" ] && [ "$codename" = "precise" ] ); then
		# https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test
		sudo add-apt-repository --yes "deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu ${codename} main"
		#sudo add-apt-repository --yes "deb-src http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu ${codename} main"
	    fi
	fi
	if [ "$distribution" = "Ubuntu" ] || [ "$distribution" = "Debian" ]; then
	    if [ "$Compiler_name" = "clang" ]; then
		# http://apt.llvm.org/
		sudo add-apt-repository --yes "deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename} main"
		#sudo add-apt-repository --yes "deb-src http://apt.llvm.org/${codename}/ llvm-toolchain-${codename} main"
		sudo add-apt-repository --yes "deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename}-${Compiler_version} main"
		#sudo add-apt-repository --yes "deb-src http://apt.llvm.org/${codename}/ llvm-toolchain-${codename}-${Compiler_version} main"

		wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
	    fi
	fi
    fi
fi
set -e

if [ "$VersionAvByDefault" = "" ]; then
    # UPDATE REPOS
    sudo apt-get update -qq
    #sudo apt-get upgrade -qq # UPGRADE SYSTEM TO LATEST PATCH LEVEL
    sudo apt-get install -y -qq
fi

if [ "$Compiler_version" != "" ] && [ "$Compiler_version" != "default" ]; then
    if [ "$Compiler_name" = "gcc" ]; then
	set +ex
	Gcc_AvSepGpp="$(apt-cache search ^g[c+][c+]-[0-9] | grep -v '[0-9]-[a-zA-Z]' | grep "^g++-$Compiler_version_grep")"
	set -ex
	if [ "$Gcc_AvSepGpp" = "" ]; then
	    sudo apt-get --allow-unauthenticated install -qq --force-yes gcc-${Compiler_version}
	else
	    sudo apt-get --allow-unauthenticated install -qq --force-yes gcc-${Compiler_version} g++-${Compiler_version}
	fi
    elif [ "$Compiler_name" = "clang" ]; then
	sudo apt-get --allow-unauthenticated install -qq --force-yes clang-${Compiler_version}
    fi
fi

# what available versions we can use
set +x
apt-cache search ^g[c+][c+]-[0-9] | grep -v '[0-9]-[a-zA-Z]'
apt-cache search ^clang-[0-9] | grep -v '[0-9]-[a-zA-Z]'
set -x

# INSTALL OUR DEPENDENCIES
sudo $SCRIPTDIR/mk/linux/setupBuildDeps.sh --quiet

if [ "$distribution" = "Ubuntu" ]; then
    case $release in
	12.04*)
	    SDL2_version="2.0.5"
	    wget https://www.libsdl.org/release/SDL2-${SDL2_version}.tar.gz
	    tar xf SDL2-${SDL2_version}.tar.gz
	    ( cd SDL2-${SDL2_version}
	    ./configure --enable-static --disable-shared
	    make
	    sudo make install )
	    ;;
	*)
	    ;;
    esac
fi
