#!/bin/bash
# Use this script to idenitfy previous and current Version for a Version Release
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2011 Mark Vejvoda under GNU GPL v3.0+

CURRENTDIR="$(dirname $(readlink -f $0))"
GOOGLE_BREAKPAD_DIR="$CURRENTDIR/../../google-breakpad/"
#$GOOGLE_BREAKPAD_DIR/src/tools/linux/dump_syms/dump_syms ./megaglest > megaglest.sym
#head -n1 megaglest.sym
python ./symbolstore.py $GOOGLE_BREAKPAD_DIR/src/tools/linux/dump_syms/dump_syms ./symbols ./megaglest
echo 'SYMBOLS FILE BUILD COMPLETE.'
echo 'To get a stack trace run:'
echo "$GOOGLE_BREAKPAD_DIR/src/processor/minidump_stackwalk ./328eaddc-c1d5-9eee-3ca1e6a4-0ce3f6a6.dmp symbols"
echo 'Where the dmp file can be replaced with your dmp file.'
