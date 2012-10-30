#!/bin/bash
# Use this script to gather MegaGlest performance stats
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2012 Mark Vejvoda under GNU GPL v3.0+

echo 'Recording performnce stats...'

perf record ./megaglest $@

perf report
