#!/bin/bash
# Use this script to prepare MegaGlest files for linux on steam
# ----------------------------------------------------------------------------
# Written by filux <heross(@@)o2.pl>
# Copyright (c) 2017 under GNU GPL v3.0+

CURRENTDIR="$(dirname "$(readlink -f "$0")")"
cd "$CURRENTDIR"

megaglest_linux_path="$CURRENTDIR/../../../../linux"
BINARY_DIR="$("$megaglest_linux_path/make-binary-archive.sh" --show-result-path2)"
DATA_DIR="$("$megaglest_linux_path/make-data-archive.sh" --show-result-path2)"

VERSION="$("$megaglest_linux_path/mg-version.sh" --version)"
kernel="$(uname -s | tr '[A-Z]' '[a-z]')"
architecture="$(uname -m  | tr '[A-Z]' '[a-z]')"

# Grab the version
echo "Linux project root path [$megaglest_linux_path]"
echo "About to prepare files for steam and for $VERSION"
# Stop if anything produces an error.
set -ex

cd "$CURRENTDIR"
INSTALLDATADIR="content/base_content"
rm -rf "$INSTALLDATADIR"; sleep 0.1s; mkdir "$INSTALLDATADIR"
if [ "$architecture" = "x86_64" ]; then INSTALLBINDIR="content/linux_x64/"
    else INSTALLBINDIR="content/linux_x86/"; fi
rm -rf "$INSTALLBINDIR"; sleep 0.1s; mkdir "$INSTALLBINDIR"

echo "Copying MegaGlest binary files..."
"$megaglest_linux_path/make-binary-archive.sh" --installer
cd "$BINARY_DIR"
cp -r * "$CURRENTDIR/$INSTALLBINDIR"
# create symlink from "lib" to "bin" where steam is able to find libs on the very end of its searching
cd "$CURRENTDIR/$INSTALLBINDIR"
ln -s "lib" "bin"

if [ "$1" != "--binary-only" ]; then
    echo "Copying MegaGlest data files..."
    "$megaglest_linux_path/make-data-archive.sh" --installer
    cd "$DATA_DIR"
    cp -r * "$CURRENTDIR/$INSTALLDATADIR"
fi

cd "$CURRENTDIR"
set +ex
echo "Successfully prepared files!"

exit 0
