#!/bin/bash

mkdir -p build
cd build

[[ -f "CMakeCache.txt" ]] && rm -f "CMakeCache.txt"
# This is for regular developers and used by our installer
cmake -DCMAKE_INSTALL_PREFIX= -DWANT_STATIC_LIBS=ON ..
make

cd ..
echo 'You may now launch MegaGlest from this directory like this:'
echo 'mk/linux/megaglest --ini-path=mk/linux/ --data-path=mk/linux/'
echo 'or from within the build directory:'
echo '../mk/linux/megaglest --ini-path=../mk/linux/ --data-path=../mk/linux/'
