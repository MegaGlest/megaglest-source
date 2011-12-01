#! /bin/sh

exec 3>&1

sudo opcontrol --reset
sudo opcontrol --start
./megaglest --load-scenario=benchmark
sudo opcontrol --shutdown
# opreport -lt1
opreport --symbols ./megaglest >perf.log
gedit perf.log
