#!/bin/bash
# Use this script to build MegaGlest Steam Shim using make
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011-2017 Mark Vejvoda under GNU GPL v3.0+

# ----------------------------------------------------------------------------
CURRENTDIR="$(dirname $(readlink -f $0))"
rm -f ${CURRENTDIR}/megaglest_shim
rm -f ${CURRENTDIR}/megaglest
# Pass the path to your steam SDK example:
# builds.sh STEAMWORKS?=/home/softcoder/Code/steamworks_sdk/sdk
#
# make STEAMWORKS?=/home/softcoder/Code/steamworks_sdk/sdk
make $@
if [ $? -ne 0 ]; then
  echo 'ERROR: MAKE failed.' >&2; exit 2
fi

STEAM_MG_PATH="$HOME/.steam/steam/steamapps/common/MegaGlest/"
if [ -d ${STEAM_MG_PATH} ]; then 
    if [ ! -f "${STEAM_MG_PATH}/steam_appid.txt" ]; then 
        echo "Installing steam appid file to Megaglest Steam folder [${STEAM_MG_PATH}] ..."
        cp ${CURRENTDIR}/steam_appid.txt ${STEAM_MG_PATH}/steam_appid.txt
    fi

    PARAM1=$1
    prefix=STEAMWORKS?=
    STEAMWORKS=${PARAM1#$prefix}
    # echo "${STEAMWORKS}"
    if [ ! -f "${STEAM_MG_PATH}/lib/libsteam_api.so" ]; then 
        echo "Installing steam sdk api to Megaglest Steam folder [${STEAM_MG_PATH}/lib/] ..."
        cp ${STEAMWORKS}/redistributable_bin/linux64/libsteam_api.so ${STEAM_MG_PATH}/lib/libsteam_api.so
    fi

    echo "Installing steam shim to Megaglest Steam folder [${STEAM_MG_PATH}] ..."
    cp ${CURRENTDIR}/megaglest_shim ${STEAM_MG_PATH}/megaglest_shim
fi
