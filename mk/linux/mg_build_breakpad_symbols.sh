#!/bin/bash
# Use this script to produce a google-breakpad symbol file for megaglest
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2013 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname $(readlink -f $0))"
SYMBOLS_DIR=${CURRENTDIR}/"linux_symbols"

mkdir -p ${SYMBOLS_DIR}
echo "Symbols folder: ${SYMBOLS_DIR}"
python ${CURRENTDIR}/symbolstore.py  ${CURRENTDIR}/../../google-breakpad/src/tools/linux/dump_syms/dump_syms ${SYMBOLS_DIR} ${CURRENTDIR}/megaglest

