#!/bin/sh

ubuntu804_32="2.6.24-27-generic"
OSTYPE=`uname -r`

sudo apt-get install build-essential subversion automake autoconf autogen jam

if [ "$OSTYPE" = $ubuntu804_32 ]; then
  sudo apt-get install libsdl1.2-dev libxerces-c2-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev
  
else
  sudo apt-get install libsdl1.2-dev libxerces28-dev libalut-dev libgl1-mesa-dev libglu1-mesa-dev libvorbis-dev libwxbase2.8-dev libwxgtk2.8-dev libx11-dev liblua5.1-0-dev libjpeg-dev libpng12-dev

fi

sudo apt-get install cmake
