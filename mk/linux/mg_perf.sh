#! /bin/sh
# Use this script to track performance while running MegaGlest
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

exec 3>&1

sudo opcontrol --reset
sudo opcontrol --start
./megaglest --load-scenario=benchmark
sudo opcontrol --shutdown
# opreport -lt1
opreport --symbols ./megaglest >perf.log
gedit perf.log
