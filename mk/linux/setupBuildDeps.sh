#!/bin/sh

ubuntu804_32="2.6.24-27-generic"
OSTYPE=`uname -m`

if [ -f /etc/fedora-release ]; then
  sudo yum groupinstall "Development Tools"
  sudo yum install subversion automake autoconf autogen jam
else
  sudo apt-get install build-essential subversion automake autoconf autogen jam
fi


if [ -f /etc/fedora-release ]; then
  echo "=====> Using build deps for fedora 13..."

  sudo yum install SDL-devel xerces-c-devel mesa-libGL-devel mesa-libGLU-devel libvorbis-devel wxBase wxGTK-devel lua-devel libjpeg-devel libpng-devel libcurl-devel openal-soft-devel

elif [ "`uname -r`" = $ubuntu804_32 ]; then
  echo "=====> Using build deps for old Ubuntu 8.04..."

  sudo apt-get install libsdl1.2-dev libxerces28-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev
  
elif [ "$OSTYPE" = "x86_64" ]; then
  echo "=====> Using build deps for 64 bit linux..."

  sudo apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev

else
  echo "=====> Using build deps for 32 bit Linux..."

  sudo apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev libcurl4-gnutls-dev

fi

if [ -f /etc/fedora-release ]; then
  sudo yum install cmake
else
  sudo apt-get install cmake
fi
