#!/bin/sh
#
# Clean up translations pulled from Transifex
# Should be applied only to resources in the 'Joomla' format
# (Currently that's the case for all resources.)
# ----------------------------------------------------------------------------
# Written by Tom Reynolds <tomreyn@megaglest.org>
# Copyright (c) 2012 Tom Reynolds under GNU GPL v3.0

for file in `find . -type f -name *.lng`
do
  # Rewrite &quot; to "
  # Remove blank space off beginning (behind '=') and end of translated strings because they belong into the code instead, and because Transifex would double them. 
  sed -i \
  -e 's/&quot;/"/g' \
  -e 's/^\([^=]*\)=\s*/\1=/' \
  -e 's/\s*$//' \
  -e 's/  / /g' \
  -e 's/⏎ /\\n/g' \
  -e 's/ \\n/\\n/g' \
  -e 's/\\n /\\n/g' \
  $file
done
