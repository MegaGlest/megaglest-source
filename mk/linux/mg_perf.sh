#!/bin/sh

echo 'Recording performnce stats...'

perf record ./megaglest $@

perf report
