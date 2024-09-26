#!/bin/sh

set -ev

# Set default workspace if not provided
WORKSPACE=${WORKSPACE:-$(pwd)}
# Check if the workspace path is absolute
if [[ "$WORKSPACE" != /* ]]; then
  echo "The workspace path must be absolute"
  exit 1
fi
test -d "$WORKSPACE"

# Set default source root if not provided
SOURCE_ROOT=${SOURCE_ROOT:-$WORKSPACE}
# Check if the source root path is absolute
if [[ "$SOURCE_ROOT" != /* ]]; then
  echo "The source root path must be absolute"
  exit 1
fi

APPIMAGE_BUILD_DIR="$SOURCE_ROOT/_build_appimage"

#if [ -n "$DO_CLEAN_BUILD" ] && [ -d $APPIMAGE_BUILD_DIR ]; then
  #rm -rf $APPIMAGE_BUILD_DIR
#fi

GAME_BUILD_DIR="$APPIMAGE_BUILD_DIR/game"
MAP_EDITOR_BUILD_DIR="$APPIMAGE_BUILD_DIR/mapeditor"
G3DVIEWER_BUILD_DIR="$APPIMAGE_BUILD_DIR/g3dviewer"
for dir in "$GAME_BUILD_DIR" "$MAP_EDITOR_BUILD_DIR" "$G3DVIEWER_BUILD_DIR"; do
  if [ ! -d "$dir" ]; then
    mkdir -p "$dir"
  fi
  if [ -d "$dir/AppDir" ]; then
    rm -rf "$dir/AppDir"
  fi
done

cd $GAME_BUILD_DIR

sudo DEBIAN_FRONTEND=noninteractive -i sh -c \
  "apt update &&
  apt upgrade -y &&
  apt install -y build-essential libcurl4-gnutls-dev libsdl2-dev libopenal-dev liblua5.3-dev libjpeg-dev libpng-dev libfreetype6-dev libwxgtk3.0-gtk3-dev libcppunit-dev libfribidi-dev libftgl-dev libglew-dev libogg-dev libvorbis-dev libminiupnpc-dev libircclient-dev libxml2-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev librtmp-dev libkrb5-dev libldap2-dev libidn2-dev libpsl-dev libgnutls28-dev libnghttp2-dev libssh-dev libbrotli-dev p7zip-full imagemagick"

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
make DESTDIR=$GAME_BUILD_DIR/AppDir -j$(nproc) install
# strip AppDir/usr/local/bin/megaglest

if [ ! -d "$GAME_BUILD_DIR/AppDir/usr/bin" ]; then
    mkdir -p "$GAME_BUILD_DIR/AppDir/usr/bin"
fi

cp $(whereis 7z | awk -F ' ' '{print $2;}') $GAME_BUILD_DIR/AppDir/usr/bin/
# Hacky workaround to use internal 7z.
sed -i 's#=7z#=$APPLICATIONPATH/7z#' AppDir/usr/local/share/megaglest/glest.ini

GAME_DESKTOP_DEST="$GAME_BUILD_DIR/AppDir/usr/local/share/applications"

if [ ! -d "$GAME_DESKTOP_DEST" ]; then
  mkdir -p "$GAME_DESKTOP_DEST"
fi

if [ ! -f "$GAME_DESKTOP_DEST/megaglest.desktop" ]; then
  curl -Lo $GAME_DESKTOP_DEST/megaglest.desktop https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/desktop/megaglest.desktop
fi

convert $GAME_BUILD_DIR/AppDir/usr/local/share/megaglest/megaglest.ico megaglest.png
mv megaglest-2.png megaglest.png

LINUXDEPLOY_OUTPUT_VERSION="$VERSION"

linuxdeploy -d $GAME_DESKTOP_DEST/megaglest.desktop \
  --icon-file=megaglest.png \
   --icon-filename=megaglest \
   --executable $GAME_BUILD_DIR/AppDir/usr/local/bin/megaglest \
   --appdir $GAME_BUILD_DIR/AppDir \
   --output appimage

mv MegaGlest*.AppImage $WORKSPACE

cd $MAP_EDITOR_BUILD_DIR

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=ON -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF -DINCLUDE_DATA=OFF
make DESTDIR=$MAP_EDITOR_BUILD_DIR/AppDir -j$(nproc) install
# strip AppDir/usr/local/bin/megaglest_editor

if [ ! -d $MAP_EDITOR_BUILD_DIR/AppDir/usr/bin ]; then
    mkdir -p $MAP_EDITOR_BUILD_DIR/AppDir/usr/bin
fi

MAP_EDITOR_DESKTOP_DEST="$MAP_EDITOR_BUILD_DIR/AppDir/usr/local/share/applications"

if [ ! -d "$MAP_EDITOR_DESKTOP_DEST" ]; then
  mkdir -p "$MAP_EDITOR_DESKTOP_DEST"
fi

if [ ! -f "$MAP_EDITOR_DESKTOP_DEST/megaglest_editor.desktop" ]; then
  curl -Lo "$MAP_EDITOR_DESKTOP_DEST/megaglest_editor.desktop" https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/desktop/megaglest_editor.desktop
  # Another stupid hack to fix icons.
  sed -i 's#Icon=megaglest#Icon=editor#' $MAP_EDITOR_DESKTOP_DEST/megaglest_editor.desktop
fi

convert $MAP_EDITOR_BUILD_DIR/AppDir/usr/local/share/megaglest/editor.ico editor.png
mv editor-2.png editor.png

linuxdeploy -d $MAP_EDITOR_DESKTOP_DEST/megaglest_editor.desktop \
  --icon-file=editor.png \
   --icon-filename=editor --executable AppDir/usr/local/bin/megaglest_editor --appdir $MAP_EDITOR_BUILD_DIR/AppDir --plugin gtk --output appimage

mv MegaGlest*.AppImage $WORKSPACE

cd $G3DVIEWER_BUILD_DIR

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=ON -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF -DINCLUDE_DATA=OFF
make DESTDIR=$G3DVIEWER_BUILD_DIR/AppDir -j$(nproc) install
# strip AppDir/usr/local/bin/megaglest_g3dviewer

G3DVIEWER_DESKTOP_DEST="$G3DVIEWER_BUILD_DIR/AppDir/usr/local/share/applications"

if [ ! -d "$G3DVIEWER_DESKTOP_DEST" ]; then
  mkdir -p "$G3DVIEWER_DESKTOP_DEST"
fi

if [ ! -f "$G3DVIEWER_DESKTOP_DEST/megaglest_g3dviewer.desktop" ]; then
  curl -Lo "$G3DVIEWER_DESKTOP_DEST/megaglest_g3dviewer.desktop" https://raw.githubusercontent.com/MegaGlest/megaglest-data/develop/others/desktop/megaglest_g3dviewer.desktop
  # Another stupid hack to fix icons.
  sed -i 's#Icon=megaglest#Icon=g3dviewer#' $G3DVIEWER_DESKTOP_DEST/megaglest_g3dviewer.desktop
fi

convert $G3DVIEWER_BUILD_DIR/AppDir/usr/local/share/megaglest/g3dviewer.ico g3dviewer.png
mv g3dviewer-2.png g3dviewer.png

linuxdeploy -d $G3DVIEWER_DESKTOP_DEST/megaglest_g3dviewer.desktop \
  --icon-file=g3dviewer.png \
   --icon-filename=g3dviewer --executable $G3DVIEWER_BUILD_DIR/AppDir/usr/local/bin/megaglest_g3dviewer --appdir AppDir --plugin gtk --output appimage

mv MegaGlest*.AppImage $WORKSPACE

# Tools
mkdir -p $APPIMAGE_BUILD_DIR/tools
cd $APPIMAGE_BUILD_DIR/tools

cmake $SOURCE_ROOT -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=ON
make -j$(nproc)

strip source/tools/glexemel/g2xml source/tools/glexemel/xml2g
mv source/tools/glexemel/g2xml $WORKSPACE
mv source/tools/glexemel/xml2g $WORKSPACE
