#!/bin/sh

set -e

HERE="$(readlink -f "$(dirname "$0")")"
SOURCE_ROOT="$HERE/.."
BUILD_DIR="${BUILD_DIR:-$HERE/_build}"

# Ensure BUILD_DIR is an absolute path
[ "${BUILD_DIR#/}" != "$BUILD_DIR" ] || { echo "Error: BUILD_DIR must be an absolute path!"; exit 1; }

# Detect OS type
OS_TYPE="$(uname -s)"

cd "$SOURCE_ROOT"
if [ ! -d "$BUILD_DIR" ]; then
  meson setup "$BUILD_DIR" "$@"
else
  meson setup --reconfigure "$BUILD_DIR" "$@"
fi

cd "$HERE"

meson compile -C "$BUILD_DIR"

GAME_BIN="$BUILD_DIR/source/glest_game/megaglest"
EDITOR_BIN="$BUILD_DIR/source/glest_map_editor/megaglest_editor"
VIEWER_BIN="$BUILD_DIR/source/g3d_viewer/megaglest_g3d_viewer"

# Determine the correct directory based on OS
if [ "$OS_TYPE" = "Darwin" ]; then
  TARGET_DIR="$HERE/macos"
else
  TARGET_DIR="$HERE/linux"
fi

# Move binaries to the correct directory
for bin in "$GAME_BIN" "$EDITOR_BIN" "$VIEWER_BIN"; do
  mv -vf "$bin" "$TARGET_DIR"
done
