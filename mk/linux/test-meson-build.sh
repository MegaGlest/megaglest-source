#!/bin/sh

set -e

HERE="$(readlink -f "$(dirname "$0")")"
SOURCE_ROOT="$HERE/../.."
BUILD_DIR="${BUILD_DIR:-$HERE/_build}"

if [ ! -d "$BUILD_DIR" ]; then
  cd "$SOURCE_ROOT"
  meson setup "$BUILD_DIR" "$@"
fi

cd "$HERE"

meson compile -C "$BUILD_DIR"

GAME_BIN="$BUILD_DIR/source/glest_game/megaglest"
EDITOR_BIN="$BUILD_DIR/source/glest_map_editor/megaglest_editor"
VIEWER_BIN="$BUILD_DIR/source/g3d_viewer/megaglest_g3d_viewer"

for bin in "$GAME_BIN" "$EDITOR_BIN" "$VIEWER_BIN"; do
  mv -vf "$bin" "$HERE"
done
