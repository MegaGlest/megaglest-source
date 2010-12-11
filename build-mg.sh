#!/bin/sh

mkdir build
cd build

cmake ..
make

echo 'You may now launch mega-glest from this folder like this:'
echo '../mk/linux/glest.bin --ini-path=../mk/linux/ --data-path=../mk/linux/'


