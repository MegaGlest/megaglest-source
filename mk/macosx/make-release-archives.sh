#!/bin/sh
# Use this script to build MegaGlest release archives for a Version Release
# ----------------------------------------------------------------------------
# 2015 Written by filux <heross(@@)o2.pl>
# Copyright (c) 2015 under GNU GPL v3.0+
# ----------------------------------------------------------------------------
LANG=C

CURRENTDIR="$(cd "$(dirname "$0")"; pwd)"
cd "$CURRENTDIR"
VERSION="$(../linux/mg-version.sh --version)"
kernel="macos"

RELEASENAME="MegaGlest-game-$kernel"
PACKAGE="$RELEASENAME-$VERSION.zip"
PACKAGE2="$RELEASENAME-$VERSION.dmg"
RELEASEDIR_ROOT="$CURRENTDIR/../../../release"
RELEASEDIR="${RELEASEDIR_ROOT}/${RELEASENAME-$VERSION}"
BINARY_DIR="$(./make-binary-archive.sh --show-result-path2)"
DATA_DIR="$(../linux/make-data-archive.sh --show-result-path2)"
APP_RES_DIR="$RELEASEDIR/MegaGlest.app/Contents/Resources"
APP_BIN_DIR="$RELEASEDIR/MegaGlest.app/Contents/MacOS"
APP_PLIST_DIR="$RELEASEDIR/MegaGlest.app/Contents"
APP_GAME_DIR="$APP_RES_DIR/megaglest-game"

if [ -d "$RELEASEDIR" ]; then rm -rf "$RELEASEDIR"; fi
mkdir -p "$APP_GAME_DIR"
mkdir -p "$APP_BIN_DIR"

./make-binary-archive.sh --installer
cp -r "$BINARY_DIR/"* "$APP_GAME_DIR"
../linux/make-data-archive.sh --installer
cp -r "$DATA_DIR/"* "$APP_GAME_DIR"; sleep 0.5s
if [ -f "$APP_GAME_DIR/MegaGlest.sh" ]; then rm -f "$APP_GAME_DIR/MegaGlest.sh"; fi

cp "$CURRENTDIR/build/mk/macosx/bundle_resources/Info.plist" "$APP_PLIST_DIR"
cp "$CURRENTDIR/bundle_resources/MegaGlest.icns" "$APP_RES_DIR"
cp "$CURRENTDIR/bundle_resources/MegaGlest.sh" "$APP_BIN_DIR"
mv "$APP_BIN_DIR/MegaGlest.sh" "$APP_BIN_DIR/MegaGlest"
mv "$APP_GAME_DIR/lib" "$APP_GAME_DIR/Frameworks"; sleep 0.5s
mv "$APP_GAME_DIR/Frameworks" "$APP_PLIST_DIR/"

echo "creating $PACKAGE"
cd "$RELEASEDIR_ROOT"
if [ -f "$PACKAGE" ]; then rm -f "$PACKAGE"; fi
cd "$RELEASENAME"
zip -9r "../$PACKAGE" "MegaGlest.app" >/dev/null
ls -lhA "${RELEASEDIR_ROOT}/$PACKAGE"

echo "creating $PACKAGE2"
cd "$CURRENTDIR/build"
if [ -f "$RELEASEDIR_ROOT/$PACKAGE2" ]; then rm -f "$RELEASEDIR_ROOT/$PACKAGE2"; fi
cpack
mv -f MegaGlest*.dmg "$RELEASEDIR_ROOT"
ls -lhA "${RELEASEDIR_ROOT}/$PACKAGE2"

cd "$RELEASEDIR_ROOT"
find "${RELEASEDIR_ROOT}" -name "MegaGlest*" -type d | xargs rm -rf 2>/dev/null
find "${RELEASEDIR_ROOT}" -name "megaglest*" -type d | xargs rm -rf 2>/dev/null
