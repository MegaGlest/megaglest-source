#!/bin/bash
# Use this script to gather MegaGlest performance stats
# ----------------------------------------------------------------------------
# Written by Mark Vejvoda <mark_vejvoda@hotmail.com>
# Copyright (c) 2012 Mark Vejvoda under GNU GPL v3.0+

echo 'Recording performnce stats...'

echo 'cat /proc/sys/kernel/kptr_restrict'
echo '1'
#echo 'echo 0 > /proc/sys/kernel/kptr_restrict'
echo  'sudo sh -c "echo 0 > /proc/sys/kernel/kptr_restrict"'
echo 'cat /proc/sys/kernel/kptr_restrict'
echo '0'

perf record ./megaglest $@

perf report
