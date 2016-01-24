#!/bin/sh

set -ex

wget https://www.libsdl.org/release/SDL2-2.0.3.tar.gz
tar xf SDL2-2.0.3.tar.gz
(
cd SDL2-2.0.3
./configure --enable-static --disable-shared
make
sudo make install
)
