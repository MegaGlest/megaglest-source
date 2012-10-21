#!/bin/sh
#
# Clean up translations pulled from Transifex
# Should be applied only to resources in the 'Joomla' format
# (Currently that's the case for all resources.)

for file in `find . -type f -name *.lng`
do
  # Rewrite &quot; to "
  # Remove blank space off beginning (behind '=') and end of translated strings because they belong into the code instead, and because Transifex would double them. 
  sed -i \
  -e 's/&quot;/"/g' \
  -e 's/^\([^=]*\)=\s*/\1=/' \
  -e 's/\s*$//' \
  $file
done
