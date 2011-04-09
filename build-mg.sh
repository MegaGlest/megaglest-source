#!/bin/sh

mkdir -p build
cd build

cmake ..
make

cd ..
echo 'You may now launch mega-glest from this directory like this:'
echo 'mk/linux/megaglest.bin --ini-path=mk/linux/ --data-path=mk/linux/'
echo 'or from within the build folder:'
echo '../mk/linux/megaglest.bin --ini-path=../mk/linux/ --data-path=../mk/linux/'
