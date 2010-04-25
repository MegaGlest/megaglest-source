#!/bin/bash

set -e
set -x

cd `hg root`
hg fetch  # will only work on a unmodified working directory.
rm -rf launchpad
mkdir $_
cd $_
tar -xzf ~/launchpad-export.tar.gz
../misc/po2localization.pl *.po mojosetup/mojosetup.pot >../scripts/localization.lua
hg diff ../scripts/localization.lua |less
set +x
echo "enter to commit, ctrl-c to abort."
read
set -x
hg commit -m "Latest translations from launchpad.net ..." ../scripts/localization.lua
cd ..
rm -rf launchpad ~/launchpad-export.tar.gz
exit 0
