#!/bin/bash
# Use this script to produce a google-breakpad stacktrace from a megaglest dmp file
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2013 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname $(readlink -f $0))"
SYMBOLS_DIR=${CURRENTDIR}/"linux_symbols"

usage(){
        echo "Syntax : $0 yourcrashfile.dmp
	echo "Example: $0 ./328eaddc-c1d5-9eee-3ca1e6a4-0ce3f6a6.dmp
	exit 1
}
 
[ $# -eq 0 ] && usage

echo "About to produce stack trace for $1"
echo "Symbols folder: ${SYMBOLS_DIR}"
${CURRENTDIR}/../../google-breakpad/src/processor/minidump_stackwalk $1 ${CURRENTDIR}/${SYMBOLS_DIR}
