#!/bin/sh
# Use this script to install build dependencies on a number of Linux platforms
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

ubuntu804_32="2.6.24-27-generic"
OSTYPE=`uname -m`
# version=$(lsb_release -i | awk '{print $3}')
version=`lsb_release -i | awk '{print $3}'`

echo "Os = [$OSTYPE] version [$version]"

if [ -f /etc/fedora-release ]; then
  sudo yum groupinstall "Development Tools"
  sudo yum install subversion automake autoconf autogen cmake
elif [ -f /etc/SuSE-release ]; then
  sudo zypper install subversion gcc gcc-c++ automake cmake
else
  sudo apt-get install build-essential subversion automake autoconf autogen cmake
fi

if [ -f /etc/SuSE-release ]; then
  echo "=====> Using build deps for Open SuSE 11.2 and above..."
  sudo zypper install libSDL-devel libxerces-c-devel MesaGLw-devel freeglut-devel libvorbis-devel wxGTK-devel lua-devel libjpeg-devel libpng14-devel libcurl-devel openal-soft-devel xorg-x11-libX11-d libxml2-devevel libircclient-dev glew-devel ftgl-devel
elif [ -f /etc/fedora-release ]; then
  echo "=====> Using build deps for fedora 13 and above..."
  sudo yum install SDL-devel xerces-c-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel libX11-devel libxml2-dev libircclient-dev glew-devel ftgl-devel
elif [ "$OSTYPE" = "i686" ] && [ "$version" = "Ubuntu" ]; then
  echo "=====> Using build deps for old Ubuntu 8.04..."
  sudo apt-get install libsdl1.2-dev libxerces28-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev libxml2-dev libircclient-dev libglew-dev
elif [ "$OSTYPE" = "x86_64" ]; then
  echo "=====> Using build deps for debian based 64 bit linux..."
  sudo apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev cmake-curses-gui libgtk2.0-dev libxml2-dev libircclient-dev libftgl-dev libminiupnpc-dev libglew-dev
else
  echo "=====> Using build deps for debian based 32 bit Linux..."
  sudo apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev cmake-curses-gui libgtk2.0-dev libxml2-dev libircclient-dev libftgl-dev libminiupnpc-dev libglew-dev
fi

