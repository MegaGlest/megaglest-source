#!/bin/sh

mkdir -p build
cd build

cmake ..
make

cd ..
echo 'You may now launch MegaGlest from this directory like this:'
echo 'mk/linux/megaglest.bin --ini-path=mk/linux/ --data-path=mk/linux/'
echo 'or from within the build directory:'
echo '../mk/linux/megaglest.bin --ini-path=../mk/linux/ --data-path=../mk/linux/'
