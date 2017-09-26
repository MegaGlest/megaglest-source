#!/bin/bash
# Use this script to build MegaGlest Steam Shim using make
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011-2013 Mark Vejvoda under GNU GPL v3.0+

# ----------------------------------------------------------------------------
rm megaglest_shim
rm megaglest
#make STEAMWORKS?=/home/softcoder/Code/steamworks_sdk/sdk
make $@
