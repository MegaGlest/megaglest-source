#!/bin/sh

# To test this script locally, you can use something like:
# docker run -it --rm -e HOSTUID=$(id -u) -e \
# -v $PWD:/workspace -w /workspace --entrypoint=bash andy5995/linuxdeploy:v2-focal
#
# Changing entrypoint above will give you a shell when the container starts.
# Then: 'su - builder' (preserve some environmental variables with '-')
# 'bash'
# 'export VERSION=snapshot'
# 'cd /workspace'
# 'mk/linux/build-appimage.sh'

set -ev

if [ -z "$VERSION" ]; then
  echo "VERSION must be set"
  exit 1
fi

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

# Used by linuxdeploy when it sets the filename
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"

APPIMAGE_BUILD_DIR="$SOURCE_ROOT/_build_appimage"
INST_PREFIX="usr"

#if [ -n "$DO_CLEAN_BUILD" ] && [ -d $APPIMAGE_BUILD_DIR ]; then
  #rm -rf $APPIMAGE_BUILD_DIR
#fi

if [ ! -d "$APPIMAGE_BUILD_DIR" ]; then
  mkdir -p "$APPIMAGE_BUILD_DIR"
fi

sudo DEBIAN_FRONTEND=noninteractive -i sh -c \
  "apt update &&
  apt upgrade -y &&
  apt install -y
  build-essential
    libcurl4-gnutls-dev
    libsdl2-dev
    libopenal-dev
    liblua5.3-dev
    libjpeg-dev
    libpng-dev
    libfreetype6-dev
    libwxgtk3.0-gtk3-dev
    libcppunit-dev
    libfribidi-dev
    libftgl-dev
    libglew-dev
    libogg-dev
    libvorbis-dev
    libminiupnpc-dev
    libircclient-dev
    libxml2-dev
    libx11-dev
    libgl1-mesa-dev
    libglu1-mesa-dev
    librtmp-dev
    libkrb5-dev
    libldap2-dev
    libidn2-dev
    libpsl-dev
    libgnutls28-dev
    libnghttp2-dev
    libssh-dev
    libbrotli-dev
    p7zip-full
    imagemagick"

cd "$APPIMAGE_BUILD_DIR"
cmake $SOURCE_ROOT \
  -DCMAKE_INSTALL_PREFIX=/$INST_PREFIX \
  -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=OFF
make DESTDIR=$APPIMAGE_BUILD_DIR/AppDir -j$(nproc) install
# This is done by linuxdeploy
# strip AppDir/$INST_PREFIX/local/bin/megaglest

cp $(whereis 7z | awk -F ' ' '{print $2;}') $APPIMAGE_BUILD_DIR/AppDir/$INST_PREFIX/bin/
# Hacky workaround to use internal 7z.
sed -i 's#=7z#=$APPLICATIONPATH/7z#' AppDir/$INST_PREFIX/share/megaglest/glest.ini

GAME_DESKTOP_DEST="$APPIMAGE_BUILD_DIR/AppDir/$INST_PREFIX/share/applications"

if [ ! -d "$GAME_DESKTOP_DEST" ]; then
  mkdir -p "$GAME_DESKTOP_DEST"
fi

if [ ! -f "$GAME_DESKTOP_DEST/megaglest.desktop" ]; then
  cp "$SOURCE_ROOT/data/glest_game/others/desktop/megaglest.desktop" "$GAME_DESKTOP_DEST"
fi

convert $APPIMAGE_BUILD_DIR/AppDir/$INST_PREFIX/share/megaglest/megaglest.ico megaglest.png
mv megaglest-2.png megaglest.png

MAP_EDITOR_DESKTOP_DEST="$APPIMAGE_BUILD_DIR/AppDir/$INST_PREFIX/share/applications"
if [ ! -d "$MAP_EDITOR_DESKTOP_DEST" ]; then
  mkdir -p "$MAP_EDITOR_DESKTOP_DEST"
fi

if [ ! -f "$MAP_EDITOR_DESKTOP_DEST/megaglest_editor.desktop" ]; then
  cp "$SOURCE_ROOT/data/glest_game/others/desktop/megaglest_editor.desktop" "$MAP_EDITOR_DESKTOP_DEST"
  # Another stupid hack to fix icons.
  sed -i 's#Icon=megaglest#Icon=editor#' $MAP_EDITOR_DESKTOP_DEST/megaglest_editor.desktop
fi

#convert $APPIMAGE_BUILD_DIR/AppDir/$INST_PREFIX/share/megaglest/editor.ico editor.png
#mv editor-2.png editor.png

G3DVIEWER_DESKTOP_DEST="$APPIMAGE_BUILD_DIR/AppDir/$INST_PREFIX/share/applications"
if [ ! -d "$G3DVIEWER_DESKTOP_DEST" ]; then
  mkdir -p "$G3DVIEWER_DESKTOP_DEST"
fi

if [ ! -f "$G3DVIEWER_DESKTOP_DEST/megaglest_g3dviewer.desktop" ]; then
  cp "$SOURCE_ROOT/data/glest_game/others/desktop/megaglest_g3dviewer.desktop" "$G3DVIEWER_DESKTOP_DEST"
  # Another stupid hack to fix icons.
  sed -i 's#Icon=megaglest#Icon=g3dviewer#' $G3DVIEWER_DESKTOP_DEST/megaglest_g3dviewer.desktop
fi

#convert $APPIMAGE_BUILD_DIR/AppDir/$INST_PREFIX/share/megaglest/g3dviewer.ico g3dviewer.png
#mv g3dviewer-2.png g3dviewer.png

linuxdeploy -d $GAME_DESKTOP_DEST/megaglest.desktop \
  --icon-file=megaglest.png \
  --icon-filename=megaglest \
  --custom-apprun="$SOURCE_ROOT/mk/linux/AppRun" \
  --executable AppDir/$INST_PREFIX/bin/megaglest \
  --appdir AppDir \
  --plugin gtk \
  --output appimage

mv MegaGlest*.AppImage $WORKSPACE

# Tools
#mkdir -p $APPIMAGE_BUILD_DIR/tools
#cd $APPIMAGE_BUILD_DIR/tools

#cmake $SOURCE_ROOT -DBUILD_MEGAGLEST=OFF -DBUILD_MEGAGLEST_MODEL_VIEWER=OFF -DBUILD_MEGAGLEST_MAP_EDITOR=OFF -DBUILD_MEGAGLEST_MODEL_IMPORT_EXPORT_TOOLS=ON
#make -j$(nproc)

#strip source/tools/glexemel/g2xml source/tools/glexemel/xml2g
#mv source/tools/glexemel/g2xml $WORKSPACE
#mv source/tools/glexemel/xml2g $WORKSPACE
